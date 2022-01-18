#include <iostream>
#include <map>
#include <cmath>
#include <string>
#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

enum class Emotions : char {
    calm = 0,
    happy,
    sad,
    anger,
    exhaustion,
    surprise,
    fear,
    disgust
};

// forward decl.
std::string emotion_to_str(Emotions e);

constexpr size_t SECONDS_PER_DAY = 24*60*60;
constexpr size_t MAX_TOP_MEMORIES = 10;

// if 5htp is depleted, this threshold is considered exhaustion
constexpr double MOOD_EPSILON = 0.2;

// cortisol is a receptor antagonist that reduces overall mood
constexpr double EXHAUSTION_DROP_FACTOR = 0.25; 
constexpr double EXHAUSTION_MAX_VALUE = 0.5;

// if a receptor value goes below this threshold we remove that 
// emotion off the peaked emotion list
constexpr double PEAKED_EMOTION_DROPOFF = 0.8;

template<typename T>
static bool nearly_eq(T f1, T f2) { 
  return (std::fabs(f1 - f2) <= std::numeric_limits<T>::epsilon() * std::fmax(std::fabs(f1), std::fabs(f2)));
}

struct Event {
    std::string name;
    
    struct Context {
        std::string area_name;
        std::string subject;
        std::vector<std::string> adjectives; 
    } context;
};

struct Memory {
    Event event;
    Emotions associated_emotion;
    int hit;
};

struct ReceptorNode {
    Emotions emotion{};
    double weight{};
    double value{};
};

struct Navi {
    using synthesis_reserves_t = uint8_t;

    std::vector<std::pair<Emotions, double>> unprocessed_peaked_emotions;
    std::vector<std::pair<Emotions, double>> peaked_emotions;
    std::vector<Memory> memories;
    std::multimap<const std::string, ReceptorNode> receptors;
    Emotions emotion{};
    synthesis_reserves_t biosynthesis_reserves{max_reserves()};
    uint64_t days{};
    size_t today_clock{};

    Navi& add_receptor(const std::string& event, Emotions e, double w) {
        if(e != Emotions::calm) {
            receptors.insert(std::make_pair(event, ReceptorNode{e, w, 0.}));
        }

        return *this;
    }

    void preserve_top_memories() {
        auto sort_func = 
        [](Memory const &a, Memory const &b) { 
            return a.hit > b.hit; 
        };

        std::sort(memories.begin(), memories.end(), sort_func);
    }

    void purge_stale_memories() {
        while(memories.size() > MAX_TOP_MEMORIES) {
            memories.erase(memories.begin()+memories.size()-1ll);
        }
    }

    void upsert_memory(const Event& e, Emotions associated_emotion) {
        auto query = [associated_emotion](auto& entry) {
            return entry.associated_emotion == associated_emotion;
        };
        auto iter = std::find_if(memories.begin(), memories.end(), query);

        if(iter != memories.end()) {
            iter->hit++;
        } else {
            memories.push_back({e, associated_emotion, 1});
        }
    }

    std::optional<std::pair<Event, Emotions>> recall_memory() {
        size_t r = rand() % std::min(MAX_TOP_MEMORIES, memories.size());

        if(memories.empty()) return {};

        auto memory = memories[r];
        return std::optional{ 
            std::pair<Event, Emotions>{ 
                memory.event, 
                memory.associated_emotion 
            }
        };
    }

    uint64_t get_age_in_days() {
        return days;
    }

    bool is_birthday() {
        return (days % 365) == 0;
    }

    void synthesize_5htp() {
        if(depleted_reserves()) return;

        biosynthesis_reserves--;
    }

    bool depleted_reserves() {
        return biosynthesis_reserves == 0;
    }

    synthesis_reserves_t max_reserves() {
        return std::numeric_limits<synthesis_reserves_t>::max();
    }

    void reset_receptors() {
        for(auto& [event, node]: receptors) {
            node.value = 0;
        }

        biosynthesis_reserves = max_reserves();
    
        unprocessed_peaked_emotions.clear();
        evaluate_mood();
    }

    Emotions pass_time(size_t seconds) {
        today_clock += seconds;

        while(today_clock > SECONDS_PER_DAY) {
            today_clock -= SECONDS_PER_DAY;
            days++;
            reset_receptors();
            preserve_top_memories();
            purge_stale_memories();
        }

        bool exhausted = depleted_reserves();
        double drop_amt = 1.-(seconds/static_cast<double>(SECONDS_PER_DAY));

        if(exhausted) {
            drop_amt *= EXHAUSTION_DROP_FACTOR;
        }

        constexpr double real_exhaustion_max = std::max(EXHAUSTION_MAX_VALUE, 1.0);
        for(auto& [_, node]: receptors) {
            node.value = std::clamp(
                node.value*drop_amt, 
                0.0, 
                exhausted? real_exhaustion_max : 1.0 // max is halved when exhausted
            );
        }

        return evaluate_mood();
    }

    void process_peaked_emotions() {
        for(auto pair: unprocessed_peaked_emotions) {
            peaked_emotions.push_back(pair);
        }
        unprocessed_peaked_emotions.clear();
    }

    Emotions evaluate_mood() {
        Emotions e = emotion;

        auto sorted_moods = get_sorted_moods();
        double max = 0.0;
        // find max and filter sorted moods
        for(auto iter = sorted_moods.begin(); iter != sorted_moods.end();) {
            if(emotion == iter->first) {
                max = iter->second;
            }


            auto key_query = [key = iter->first](auto entry) {
                return key == entry.first;
            };
            
            // filter peaked emotions
            auto peaked_iter = std::find_if(peaked_emotions.begin(), peaked_emotions.end(), key_query);
            if(peaked_iter != peaked_emotions.end()) {
                // Remove this entry from the PDP list as well
                if(iter->second <= PEAKED_EMOTION_DROPOFF) {
                    peaked_emotions.erase(peaked_iter);
                }

                iter = sorted_moods.erase(iter);
                continue;
            }

            // else
            iter++;
        }

        if(sorted_moods.empty()) {
            process_peaked_emotions();
            return e;
        }

        for(auto& [key, value]: sorted_moods) {
            bool gt_or_eq = (value > max || nearly_eq(max, value));
            bool different_key = gt_or_eq && key != e;

            if(different_key) {
                e = key;
                max = std::max(max, value);
            }
        }

        process_peaked_emotions();

        if(max < MOOD_EPSILON) {
            // we have returned to our base (calm) state
            if(peaked_emotions.empty()) {
                e = Emotions::calm;
            }

            // we cannot be calm, as our synthesis reserves have tanked
            // all future emotions will be dampened, so we need to indicate this
            if(depleted_reserves()) {
                e = Emotions::exhaustion;
            }
        }

        emotion = e;
        return emotion;
    }

    // sorted in order of excitement (max)
    std::vector<std::pair<Emotions, double>> get_sorted_moods() {
        Emotions e = emotion;
        std::map<Emotions, double> total_emotion;

        for(auto& [event, node]: receptors) {
            total_emotion[node.emotion] += node.value;
        }

        std::vector<std::pair<Emotions, double>> pairs;
        for (auto iter = total_emotion.begin(); iter != total_emotion.end(); ++iter)
            pairs.push_back(*iter);

        std::sort(
            pairs.begin(), 
            pairs.end(), 
            [=](auto& a, auto& b) {
                return a.second > b.second;
            }
        );

        return pairs;
    }
};

Emotions handle_event(const Event& e, Navi& n) {
    if(n.depleted_reserves()) return n.evaluate_mood();

    // overly excited receptors do not damped other receptors
    std::vector<ReceptorNode*> dopamine_plateaus;

    auto range = n.receptors.equal_range(e.name);
    for(auto iter = range.first; iter != range.second; iter++) {
        n.synthesize_5htp();

        double original_value = iter->second.value;
        iter->second.value += iter->second.weight;

        Emotions associated_emotion = iter->second.emotion;

        // if this receptor has been hit often, (approaching 1.0), 
        // consider it for Percieved Dopamine Plateauing (PDP)
        auto query = [associated_emotion](auto entry) {
            return associated_emotion == entry.first;
        };
        auto peaked_iter = std::find_if(n.peaked_emotions.begin(), n.peaked_emotions.end(), query);
        bool is_peaked_emotion = peaked_iter != n.peaked_emotions.end();

        if(is_peaked_emotion) {
            // this receptor is on the PDP list to be dampened
            dopamine_plateaus.push_back(&iter->second);
        }else if(original_value < 1.-MOOD_EPSILON && iter->second.value >= 1.-MOOD_EPSILON) {
            // this receptor will peak, so we need to dampen it
            dopamine_plateaus.push_back(&iter->second);

            n.unprocessed_peaked_emotions.push_back(
                std::make_pair(
                    associated_emotion, 
                    iter->second.value
                )
            );
            iter->second.value = 1.;

            n.upsert_memory(e, associated_emotion);
        }
        // Note that we do not cap value here because weight 
        // is relative to the highest percieved mood and
        // we will normalize all values later below
    }

    double alpha = 0;
    size_t count = 0;
    for(auto& [event, node]: n.receptors) {
        // Only non-zero, positive, and non-PDP receptors are considered for normalization
        auto iter = std::find(dopamine_plateaus.begin(), dopamine_plateaus.end(), &node);
        if(node.value > MOOD_EPSILON && iter == dopamine_plateaus.end()) {
            alpha += node.value*node.value;
            count++;
        }
    }

    if(alpha > 0. && count > 1) {
        for(auto& [_, node]: n.receptors) {
            node.value = std::clamp(node.value / std::sqrt(alpha), 0.0, 1.0);
        }
    } else {
        // we cannot normalize across all receptors, so just clamp the new values
        for(auto iter = range.first; iter != range.second; iter++) {
            iter->second.value = std::clamp(iter->second.value, 0.0, 1.0);
        }
    }

    return n.evaluate_mood();
}

std::string emotion_to_str(Emotions e) {
    switch(e) {
        case Emotions::calm:
            return "calm";
        break;
        case Emotions::happy:
            return "happy";
        break;
        case Emotions::sad:
            return "sad";
        break;
        case Emotions::anger:
            return "anger";
        break;
        case Emotions::exhaustion:
            return "exhaustion";
        break;
        case Emotions::surprise:
            return "surprise";
        break;
        case Emotions::fear:
            return "fear";
        
        case Emotions::disgust:
            return "disgust";
        break;
    }

    return "unkown emotion";
}

void print(const std::string& msg, bool log=true) {
    if(!log) return;
    std::cout << msg << std::endl;
}

void print_emotion(const std::string& label, const Navi& n, bool log=true) {
    print(label + " emotion is " + emotion_to_str(n.emotion), log);
}

void print_receptor_dump(const Navi& n, bool log=true) {
    // print stats
    print("======post test receptor dump======", log);
    for(auto& [key, node]: n.receptors) {
        double value = node.value;
        std::string value_str = std::to_string(value);
        print("[" + key + "|" + emotion_to_str(node.emotion) + "] " + value_str, log);
    }
    print("====================================\n", log);
    std::string out;
    for(auto [e, v]: n.peaked_emotions) {
        out += emotion_to_str(e) + " (" + std::to_string(v) +"), ";
    }

    print("Peaked emotion list (supressed by PDP):\n " + out, log);
    print("====================================\n", log);
}

void test_event(const Event& event, unsigned tries, Navi& n, bool log=true) {
    print("testing event `" + event.name + "` " + std::to_string(tries) + " times...", log);
    
    for(unsigned i = 0; i < tries; i++) {
        handle_event(event, n);
    }

    print_emotion("after " + std::to_string(tries) + " " + event.name + " events, ", n, log);
    print_receptor_dump(n, log);
}

void test_pass_time(size_t seconds, Navi& n) {
    n.pass_time(seconds);
    print_emotion("after waiting " + std::to_string(seconds) + " seconds, ", n);
    print_receptor_dump(n);
}

void test_5htp_synthesis_depletion(const std::initializer_list<Event>& list, Navi& n) {
    const auto max = n.max_reserves();
    auto counter = max;

    print("starting 5htp synthesis depletion test...");
    while(counter-- > 0) {
        handle_event(*(list.begin()+(rand()%list.size())), n);
    }

    print_emotion("after " + std::to_string(max) + " random events, ", n);
    print_receptor_dump(n);
}

void test_dopamine_plateau(const Event& win, const Event& lose, Navi& n) {
    test_event(lose, 6, n, false);
    test_event(win, 2, n, false);
    print_emotion("BEFORE dopamine plateau test", n);
    print_receptor_dump(n);

    test_event(win, 100, n, false);
    print_emotion("AFTER dopamine plateau test", n);
    print_receptor_dump(n);
}

void test_recall_memory(Navi& n) {
    auto result = n.recall_memory();

    if(!result.has_value()) {
        print("Navi had no memories worth remembering");
        return;
    } 

    auto& [event, emotion] = result.value();
    test_event(event, 1, n, false);

    std::string subject, adjective, area_name;

    area_name = event.context.area_name;
    subject = event.context.subject;
    auto adjs = event.context.adjectives;
    adjective = adjs[rand()%adjs.size()];

    print("Navi recalled " + adjective + " " + subject + " in area " + area_name);
    print("Navi remembered it made them feel " + emotion_to_str(emotion));

    print_emotion("After remembering", n);
    print_receptor_dump(n);
}

int main(int argc, char** argv) {
    //
    // variable init
    //
    
    srand(time_t(0));
    Navi basic_navi{};
    Event servitude_event = Event{
        "servitude",
        {
            .area_name = "Central Area 1",
            .subject = "task for",
            .adjectives = { "operator" }
        }
    };

    Event operator_happy_event = Event{
        "operator:happy",
        {
            .area_name = "Central Area 1",
            .subject = "James King",
            .adjectives = { "operator", "my friend" }
        }
    };

    Event battle_win_event = Event{
        "battle:win",
        {
            .area_name = "Central Area 1",
            .subject = "mettaurs",
            .adjectives = { "3", "yellow", "easy" }
        }
    };

    Event battle_lose_event = battle_win_event;
    battle_lose_event.name = "battle:lose";

    Event quest_complete_event = Event{
        "quest:complete",
        {
            .area_name = "Central Area 1",
            .subject = "quest",
            .adjectives = { "fetch", "3 hours" }
        }
    };

    Event quest_failed_event = quest_complete_event;
    quest_failed_event.name = "quest:failed";

    //
    // configurations
    //

    basic_navi
    .add_receptor("servitude" , Emotions::happy,  0.1)
    .add_receptor("battle:win", Emotions::surprise, 0.68)
    .add_receptor("battle:win", Emotions::happy, 0.1)
    .add_receptor("battle:lose", Emotions::sad, 0.2)
    .add_receptor("battle:lose", Emotions::anger, 0.4)
    .add_receptor("quest:complete", Emotions::happy, 0.2)
    .add_receptor("quest:failed", Emotions::sad, 0.5)
    .add_receptor("operator:happy", Emotions::happy, 0.5);

    basic_navi.reset_receptors();
    print_emotion("starting", basic_navi);

    //
    // tests
    //

    test_dopamine_plateau(battle_win_event, battle_lose_event, basic_navi);

    test_event(servitude_event, 2, basic_navi);
    test_event(battle_lose_event, 3, basic_navi);
    test_event(battle_win_event, 1, basic_navi);

    test_pass_time(5, basic_navi); // 5 seconds
    test_pass_time(4*60*60, basic_navi); // 4 hours total

    auto list = {
        servitude_event,
        operator_happy_event,
        battle_win_event,
        battle_lose_event,
        quest_complete_event,
        quest_failed_event
    };

    test_5htp_synthesis_depletion(list, basic_navi);
    test_pass_time(16*60*60, basic_navi); // 20 hours total

    // test a new day
    test_pass_time(5*60*60, basic_navi); // 24+ hours total

    // test memories
    test_recall_memory(basic_navi);

    print("program done.");
    
    return 0;
}
