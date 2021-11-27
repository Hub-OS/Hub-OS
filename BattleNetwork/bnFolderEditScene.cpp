#include <time.h>
#include <cmath>

#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>
#include <Swoosh/Timer.h>

#include "bnFolderEditScene.h"
#include "Segues/BlackWashFade.h"
#include "bnCardLibrary.h"
#include "bnCardFolder.h"
#include "bnCardPackageManager.h"
#include "Android/bnTouchArea.h"

#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;

// 1. This algorithm line breaks if a whole line goes over 9 characters
// 2. When it breaks a new line, it will cleanup redundant space characters around it
// 3. If no linebreak can be cleanly placed at the last word start position,
//    it will break that word where the new line must start
// 4. To finish off, if the char limit goes over 29, then it will trim the string
std::string FolderEditScene::FormatCardDesc(const std::string&& desc)
{
    std::string output = desc;

    auto bothAreSpaces = [](char lhs, char rhs) -> bool { return (lhs == rhs) && (lhs == ' '); };

    std::string::iterator new_end = std::unique(output.begin(), output.end(), bothAreSpaces);
    output.erase(new_end, output.end());

    std::string escaped_newline = "\\n";
    std::string newline = "\n";

    std::string::size_type i = output.find(escaped_newline);

    while (i != std::string::npos) {
        output.erase(i, escaped_newline.length());
        output.insert(i, " "); // replace with space
        i = output.find(escaped_newline);
    }

    /* std::string::size_type */ i = output.find(newline);

    while (i != std::string::npos) {
        output.erase(i, newline.length());
        output.insert(i, " "); // replace with space
        i = output.find(newline);
    }

    int index = 0;
    int perLine = 0;
    int line = 0;
    int wordIndex = 0; // If we are breaking on a word

    bool finished = false;

    while (index != output.size() && !finished) {

        // Only allow 9 letters
        if (perLine > 0 && perLine % 9 == 0) {
            if (wordIndex > -1) {
                if (index == wordIndex + 8) {
                    wordIndex = index; // We have no choice but to cut off part of this lengthy word
                }
                // Line break at the last word if we continue...
                if (output[index] != ' ') {
                    output.insert(wordIndex, "\n");
                    index = wordIndex; // Start counting from here
                    while (output[index] == ' ') { output.erase(index, 1); }
                }
                else {
                    output.insert(index, "\n"); index++;
                    while (output[index] == ' ') { output.erase(index, 1); }
                }
            }
            else {
                // Line break at the next word
                while (output[index - 1] == ' ') { output.erase(index - 1, 1); index--; }
                output.insert(index, "\n"); index++;
                while (output[index] == ' ') { output.erase(index, 1); }
            }
            line++;

            if (line == 3) {
                output = output.substr(0, index + 1);
                output[output.size() - 1] = '-'; // TODO: font glyph will show an elipses
                finished = true;
            }

            perLine = -1;
            wordIndex = -1;
        }

        if (output[index] != ' ' && wordIndex == -1) {
            wordIndex = index;
        }
        else if (output[index] == ' ' && wordIndex > -1) {
            wordIndex = -1;
        }

        perLine++;
        index++;
    }

    // Card docks only fit 9 chars per line + 2 possible linebreak chars
    if (output.size() > (3 * 9) + 2) {
        output = output.substr(0, (3 * 9) + 2);
        output[output.size() - 1] = '-'; // TODO: font glyph will show an elipses
    }

    return output;
}

FolderEditScene::FolderEditScene(swoosh::ActivityController& controller, CardFolder& folder) :
    Scene(controller),
    camera(sf::View(sf::Vector2f(240, 160), sf::Vector2f(480, 320))),
    folder(folder),
    hasFolderChanged(false),
    card(),
    font(Font::Style::wide),
    menuLabel("FOLDER EDIT", font),
    cardFont(Font::Style::thick),
    cardLabel("", cardFont),
    cardDescFont(Font::Style::thin),
    cardDesc("", cardDescFont),
    numberFont(Font::Style::thick),
    numberLabel(Font::Style::gradient)
{
    // Move card data into their appropriate containers for easier management
    PlaceFolderDataIntoCardSlots();
    PlaceLibraryDataIntoBuckets();

    // We must account for existing card data to accurately represent what's left from our pool
    ExcludeFolderDataFromPool();

    // Menu name font
    menuLabel.setPosition(sf::Vector2f(20.f, 8.0f));
    menuLabel.setScale(2.f, 2.f);

    // Selection input delays
    maxSelectInputCooldown = 0.5; // half of a second
    selectInputCooldown = maxSelectInputCooldown;

    // Battle::Card UI font
    cardLabel.setPosition(275.f, 15.f);
    cardLabel.setScale(2.f, 2.f);

    numberLabel.setScale(1.6, 1.6);

    // Battle::Card description font
    cardDesc.SetColor(sf::Color::Black);
    cardDesc.setScale(2.f, 2.f);

    // folder menu graphic
    bg = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_VIEW_BG));
    bg.setScale(2.f, 2.f);

    folderDock = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_DOCK));
    folderDock.setScale(2.f, 2.f);
    folderDock.setPosition(2.f, 30.f);

    packDock = sf::Sprite(*Textures().LoadFromFile(TexturePaths::PACK_DOCK));
    packDock.setScale(2.f, 2.f);
    packDock.setPosition(480.f, 30.f);

    scrollbar = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_SCROLLBAR));
    scrollbar.setScale(2.f, 2.f);

    folderCursor = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_CURSOR));
    folderCursor.setScale(2.f, 2.f);
    folderCursor.setPosition((2.f * 90.f), 64.0f);
    folderSwapCursor = folderCursor;

    packCursor = folderCursor;
    packCursor.setPosition((2.f * 90.f) + 480.0f, 64.0f);
    packSwapCursor = packCursor;

    mbPlaceholder = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_MB));
    mbPlaceholder.setScale(2.f, 2.f);

    folderNextArrow = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_NEXT_ARROW));
    folderNextArrow.setScale(2.f, 2.f);

    packNextArrow = folderNextArrow;
    packNextArrow.setScale(-2.f, 2.f);

    folderCardCountBox = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_SIZE));
    folderCardCountBox.setPosition(sf::Vector2f(425.f, 10.f + folderCardCountBox.getLocalBounds().height));
    folderCardCountBox.setScale(2.f, 2.f);
    folderCardCountBox.setOrigin(folderCardCountBox.getLocalBounds().width / 2.0f, folderCardCountBox.getLocalBounds().height / 2.0f);

    cardHolder = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_CHIP_HOLDER));
    cardHolder.setScale(2.f, 2.f);

    packCardHolder = sf::Sprite(*Textures().LoadFromFile(TexturePaths::FOLDER_CHIP_HOLDER));
    packCardHolder.setScale(2.f, 2.f);

    element = sf::Sprite(*Textures().LoadFromFile(TexturePaths::ELEMENT_ICON));
    element.setScale(2.f, 2.f);

    // Current card graphic
    card = sf::Sprite();
    card.setTextureRect(sf::IntRect(0, 0, 56, 48));
    card.setScale(2.f, 2.f);
    card.setOrigin(card.getLocalBounds().width / 2.0f, card.getLocalBounds().height / 2.0f);

    cardIcon = sf::Sprite();
    cardIcon.setTextureRect(sf::IntRect(0, 0, 14, 14));
    cardIcon.setScale(2.f, 2.f);

    cardRevealTimer.start();
    easeInTimer.start();

    /* folder view */
    folderView.maxCardsOnScreen = 7;
    folderView.numOfCards = int(folderCardSlots.size());

    /* library view */
    packView.maxCardsOnScreen = 7;
    packView.numOfCards = int(poolCardBuckets.size());

    prevViewMode = currViewMode = ViewMode::folder;

    totalTimeElapsed = frameElapsed = 0.0;

    canInteract = false;

    setView(sf::Vector2u(480, 320));
}

FolderEditScene::~FolderEditScene() { ; }

void FolderEditScene::onStart() {
    canInteract = true;
}

void FolderEditScene::onUpdate(double elapsed) {
    frameElapsed = elapsed;
    totalTimeElapsed += elapsed;

    cardRevealTimer.update(sf::seconds(static_cast<float>(elapsed)));
    easeInTimer.update(sf::seconds(static_cast<float>(elapsed)));

    auto offset = camera.GetView().getCenter().x - 240;
    bg.setPosition(offset, 0.f);
    menuLabel.setPosition(offset + 20.0f, 8.f);

    // We need to move the view based on the camera pos
    camera.Update((float)elapsed);
    setView(camera.GetView());

    // Scene keyboard controls
    if (canInteract) {
        CardView* view = nullptr;

        if (currViewMode == ViewMode::folder) {
            view = &folderView;
        }
        else if (currViewMode == ViewMode::pool) {
            view = &packView;
        }

        if (Input().Has(InputEvents::pressed_ui_up) || Input().Has(InputEvents::held_ui_up)) {
            if (lastKey != InputEvents::pressed_ui_up) {
                lastKey = InputEvents::pressed_ui_up;
                extendedHold = false;
            }

            selectInputCooldown -= elapsed;


            if (selectInputCooldown <= 0) {
                if (!extendedHold) {
                    selectInputCooldown = maxSelectInputCooldown;
                    extendedHold = true;
                }

                if (--view->currCardIndex >= 0) {
                    Audio().Play(AudioType::CHIP_SELECT);
                    cardRevealTimer.reset();
                }

                if (view->currCardIndex < view->lastCardOnScreen) {
                    --view->lastCardOnScreen;
                }

            }
        }
        else if (Input().Has(InputEvents::pressed_ui_down) || Input().Has(InputEvents::held_ui_down)) {
            if (lastKey != InputEvents::pressed_ui_down) {
                lastKey = InputEvents::pressed_ui_down;
                extendedHold = false;
            }

            selectInputCooldown -= elapsed;

            if (selectInputCooldown <= 0) {
                if (!extendedHold) {
                    selectInputCooldown = maxSelectInputCooldown;
                    extendedHold = true;
                }

                if (++view->currCardIndex < 30) {
                    Audio().Play(AudioType::CHIP_SELECT);
                    cardRevealTimer.reset();
                }

                if (view->currCardIndex > view->lastCardOnScreen + view->maxCardsOnScreen - 1) {
                    ++view->lastCardOnScreen;
                }
            }
        }
        else if (Input().Has(InputEvents::pressed_shoulder_left)) {
            extendedHold = false;

            selectInputCooldown -= elapsed;

            if (selectInputCooldown <= 0) {
                selectInputCooldown = maxSelectInputCooldown;
                view->currCardIndex -= view->maxCardsOnScreen;

                view->currCardIndex = std::max(view->currCardIndex, 0);

                Audio().Play(AudioType::CHIP_SELECT);

                while (view->currCardIndex < view->lastCardOnScreen) {
                    --view->lastCardOnScreen;
                }

                cardRevealTimer.reset();
            }
        }
        else if (Input().Has(InputEvents::pressed_shoulder_right)) {
            extendedHold = false;

            selectInputCooldown -= elapsed;

            if (selectInputCooldown <= 0) {
                selectInputCooldown = maxSelectInputCooldown;
                view->currCardIndex += view->maxCardsOnScreen;
                Audio().Play(AudioType::CHIP_SELECT);

                view->currCardIndex = std::min(view->currCardIndex, view->numOfCards - 1);

                while (view->currCardIndex > view->lastCardOnScreen + view->maxCardsOnScreen - 1) {
                    ++view->lastCardOnScreen;
                }

                cardRevealTimer.reset();
            }
        }
        else {
            selectInputCooldown = 0;
            extendedHold = false;
        }

        if (Input().Has(InputEvents::pressed_confirm)) {
            if (currViewMode == ViewMode::folder) {
                if (folderView.swapCardIndex != -1) {
                    if (folderView.swapCardIndex == folderView.currCardIndex) {
                        Battle::Card copy;
                        bool found = false;
                        for (auto i = 0; i < poolCardBuckets.size(); i++) {
                            if (poolCardBuckets[i].ViewCard() == folderCardSlots[folderView.currCardIndex].ViewCard()) {
                                poolCardBuckets[i].AddCard();
                                folderCardSlots[folderView.currCardIndex].GetCard(copy);
                                found = true;
                                break;
                            };
                        }
                        if (found == false) {
                            folderCardSlots[folderView.currCardIndex].GetCard(copy);
                            if (copy.GetShortName().length() > 0) {
                                auto slot = PoolBucket(1, copy);
                                poolCardBuckets.push_back(slot);
                                packView.numOfCards++;
                                CHIPLIB.AddCard(copy);
                                bool gotCard = true;
                            }
                            else {
                                Audio().Play(AudioType::CHIP_ERROR);
                            }
                        }
                        // Unselect the card
                        folderView.swapCardIndex = -1;
                        Audio().Play(AudioType::CHIP_CANCEL);
                    }
                    else {
                        // swap the card
                        auto temp = folderCardSlots[folderView.swapCardIndex];
                        folderCardSlots[folderView.swapCardIndex] = folderCardSlots[folderView.currCardIndex];
                        folderCardSlots[folderView.currCardIndex] = temp;
                        Audio().Play(AudioType::CHIP_CONFIRM);

                        folderView.swapCardIndex = -1;
                    }
                }
                else {
                    // See if we're swapping from a pack
                    if (packView.swapCardIndex != -1) {
                        // Try to swap the card with the one from the pack
                        // The card from the pack is copied and added to the slot
                        // The slot card needs to find its corresponding bucket and increment it
                        Battle::Card copy;
                        Battle::Card copy2 = poolCardBuckets[packView.swapCardIndex].ViewCard();
                        Battle::Card checkCard;
                        int maxCards = copy2.GetLimit();
                        int foundCards = 0;
                        for (auto i = 0; i < folderCardSlots.size(); i++) {
                            checkCard = Battle::Card(folderCardSlots[i].ViewCard());
                            if (checkCard.GetShortName() == copy2.GetShortName() && checkCard.GetClass() == copy2.GetClass() && checkCard.GetElement() == copy2.GetElement()) {
                                foundCards++;
                            }
                        }
                        bool gotCard = false;
                        if (foundCards < maxCards || maxCards == 0) {

                            // If the pack pointed to is the same as the card in our folder, add the card back into folder
                            if (poolCardBuckets[packView.swapCardIndex].ViewCard() == folderCardSlots[folderView.currCardIndex].ViewCard()) {
                                poolCardBuckets[packView.swapCardIndex].AddCard();
                                folderCardSlots[folderView.currCardIndex].GetCard(copy);

                                gotCard = true;
                            }
                            else if (poolCardBuckets[packView.swapCardIndex].GetCard(copy)) {
                                Battle::Card prev;

                                bool findBucket = folderCardSlots[folderView.currCardIndex].GetCard(prev);

                                folderCardSlots[folderView.currCardIndex].AddCard(copy);

                                // If the card slot had a card, find the corresponding bucket to add it back into
                                if (findBucket) {
                                    auto iter = std::find_if(poolCardBuckets.begin(), poolCardBuckets.end(),
                                        [&prev](const PoolBucket& in) { return prev.GetShortName() == in.ViewCard().GetShortName(); }
                                    );

                                    if (iter != poolCardBuckets.end()) {
                                        iter->AddCard();
                                    }
                                }

                                gotCard = true;
                            }
                        }
                        if (gotCard) {
                            hasFolderChanged = true;

                            packView.swapCardIndex = -1;
                            folderView.swapCardIndex = -1;

                            Audio().Play(AudioType::CHIP_CONFIRM);
                        }
                        else {
                            Audio().Play(AudioType::CHIP_ERROR);
                        }
                    }
                    else {
                        // select the card
                        folderView.swapCardIndex = folderView.currCardIndex;
                        Audio().Play(AudioType::CHIP_CHOOSE);
                    }
                }
            }
            else if (currViewMode == ViewMode::pool) {
                if (packView.swapCardIndex != -1) {
                    if (packView.swapCardIndex == packView.currCardIndex) {
                        if (folder.GetSize() <= 30) {
                            int found = -1;
                            for (auto i = 0; i < 30; i++) {
                                if (found == -1 && folderCardSlots[i].IsEmpty()) {
                                    found = i;
                                }
                            }
                            Battle::Card copy;
                            if (found != -1 && poolCardBuckets[packView.currCardIndex].GetCard(copy)) {
                                Battle::Card checkCard;
                                int maxCards = copy.GetLimit();
                                int foundCards = 0;
                                for (auto i = 0; i < folderCardSlots.size(); i++) {
                                    checkCard = Battle::Card(folderCardSlots[i].ViewCard());
                                    if (checkCard.GetShortName() == copy.GetShortName() && checkCard.GetClass() == copy.GetClass() && checkCard.GetElement() == copy.GetElement()) {
                                        foundCards++;
                                    }
                                }
                                if (foundCards < maxCards || maxCards == 0) {
                                    folderCardSlots[found].AddCard(copy);
                                    bool gotCard = true;
                                    packView.swapCardIndex = -1;
                                    folderView.swapCardIndex = -1;
                                    hasFolderChanged = true;
                                    folderView.numOfCards++;
                                    Audio().Play(AudioType::CHIP_CONFIRM);
                                }
                                else {
                                    poolCardBuckets[packView.currCardIndex].AddCard();
                                    bool gotCard = true;
                                    hasFolderChanged = true;
                                    packView.swapCardIndex = -1;
                                    folderView.swapCardIndex = -1;
                                    Audio().Play(AudioType::CHIP_ERROR);
                                }
                            }
                        }
                        else {
                            // Unselect the card
                            packView.swapCardIndex = -1;
                            Audio().Play(AudioType::CHIP_CANCEL);
                        }
                    }
                    else {
                        // swap the pack
                        auto temp = poolCardBuckets[packView.swapCardIndex];
                        poolCardBuckets[packView.swapCardIndex] = poolCardBuckets[packView.currCardIndex];
                        poolCardBuckets[packView.currCardIndex] = temp;
                        Audio().Play(AudioType::CHIP_CONFIRM);
                        packView.swapCardIndex = -1;
                    }
                }
                else {
                    // See if we're swapping from our folder
                    if (folderView.swapCardIndex != -1 && packView.numOfCards > 0) {
                        // Try to swap the card with the one from the folder
                        // The card from the pack is copied and added to the slot
                        // The slot card needs to find its corresponding bucket and increment it
                        Battle::Card copy;

                        bool gotCard = false;

                        // If the pack pointed to is the same as the card in our folder, add the card back into folder
                        if (poolCardBuckets[packView.currCardIndex].ViewCard() == folderCardSlots[folderView.swapCardIndex].ViewCard()) {
                            poolCardBuckets[packView.currCardIndex].AddCard();
                            folderCardSlots[folderView.swapCardIndex].GetCard(copy);

                            gotCard = true;
                        }
                        else if (poolCardBuckets[packView.swapCardIndex].GetCard(copy)) {
                            Battle::Card prev;

                            bool findBucket = folderCardSlots[folderView.currCardIndex].GetCard(prev);

                            folderCardSlots[folderView.currCardIndex].AddCard(copy);

                            // If the card slot had a card, find the corresponding bucket to add it back into
                            if (findBucket) {
                                auto iter = std::find_if(poolCardBuckets.begin(), poolCardBuckets.end(),
                                    [&prev](const PoolBucket& in) { return prev.GetShortName() == in.ViewCard().GetShortName(); }
                                );

                                if (iter != poolCardBuckets.end()) {
                                    iter->AddCard();
                                }
                            }
                            gotCard = true;
                        }

                        if (gotCard) {
                            packView.swapCardIndex = -1;
                            folderView.swapCardIndex = -1;

                            hasFolderChanged = true;

                            Audio().Play(AudioType::CHIP_CONFIRM);
                        }
                        else {
                            Audio().Play(AudioType::CHIP_ERROR);
                        }
                    }
                    else {
                        // select the card
                        packView.swapCardIndex = packView.currCardIndex;
                        Audio().Play(AudioType::CHIP_CHOOSE);
                    }
                }
            }
        }
        else if (Input().Has(InputEvents::pressed_ui_right) && currViewMode == ViewMode::folder) {
            currViewMode = ViewMode::pool;
            canInteract = false;
            Audio().Play(AudioType::CHIP_DESC);
        }
        else if (Input().Has(InputEvents::pressed_ui_left) && currViewMode == ViewMode::pool) {
            currViewMode = ViewMode::folder;
            canInteract = false;
            Audio().Play(AudioType::CHIP_DESC);
        }

        view->currCardIndex = std::max(0, view->currCardIndex);
        view->currCardIndex = std::min(view->numOfCards - 1, view->currCardIndex);

        switch (currViewMode) {
        case ViewMode::folder:
        {
            using SlotType = decltype(folderCardSlots)::value_type;
            RefreshCurrentCardDock<SlotType>(*view, folderCardSlots);
        }
        break;
        case ViewMode::pool:
        {
            using SlotType = decltype(poolCardBuckets)::value_type;
            RefreshCurrentCardDock<SlotType>(*view, poolCardBuckets);
        }
        break;
        default:
            Logger::Logf("No applicable view mode for folder edit scene: %i", static_cast<int>(currViewMode));
            break;
        }

        view->prevIndex = view->currCardIndex;

        view->lastCardOnScreen = std::max(0, view->lastCardOnScreen);
        view->lastCardOnScreen = std::min(view->numOfCards - 1, view->lastCardOnScreen);

        bool gotoLastScene = false;

        if (Input().Has(InputEvents::pressed_cancel) && canInteract) {
            if (packView.swapCardIndex != -1 || folderView.swapCardIndex != -1) {
                Audio().Play(AudioType::CHIP_DESC_CLOSE);
                packView.swapCardIndex = folderView.swapCardIndex = -1;
            }
            else {
                WriteNewFolderData();

                gotoLastScene = true;
                canInteract = false;

                Audio().Play(AudioType::CHIP_DESC_CLOSE);
            }
        }

        if (gotoLastScene) {
            canInteract = false;

            using namespace swoosh::types;
            using segue = segue<BlackWashFade, milli<500>>;
            getController().pop<segue>();
        }
    } // end if(gotoLastScene)
    else {
        if (prevViewMode != currViewMode) {
            if (currViewMode == ViewMode::folder) {
                if (camera.GetView().getCenter().x > 240) {
                    camera.OffsetCamera(sf::Vector2f(-960.0f * 2.0f * float(elapsed), 0));
                }
                else {
                    prevViewMode = currViewMode;
                    canInteract = true;
                }
            }
            else if (currViewMode == ViewMode::pool) {
                if (camera.GetView().getCenter().x < 720) {
                    camera.OffsetCamera(sf::Vector2f(960.0f * 2.0f * float(elapsed), 0));
                }
                else {
                    prevViewMode = currViewMode;
                    canInteract = true;
                }
            }
            else {
                prevViewMode = currViewMode;
                canInteract = true;
            }
        }
    }
}

void FolderEditScene::onLeave() {

}

void FolderEditScene::onExit()
{
}

void FolderEditScene::onEnter()
{
    folderView.currCardIndex = 0;
    RefreshCurrentCardDock(folderView, folderCardSlots);
}

void FolderEditScene::onResume() {

}

void FolderEditScene::onDraw(sf::RenderTexture& surface) {
    surface.draw(bg);
    surface.draw(menuLabel);

    float scale = 0.0f;

    surface.draw(folderCardCountBox);

    if (int(0.5 + folderCardCountBox.getScale().y) == 2) {
        auto nonempty = (decltype(folderCardSlots))(folderCardSlots.size());
        auto iter = std::copy_if(folderCardSlots.begin(), folderCardSlots.end(), nonempty.begin(), [](auto in) { return !in.IsEmpty(); });
        nonempty.resize(std::distance(nonempty.begin(), iter));  // shrink container to new size

        std::string str = std::to_string(nonempty.size());
        
        // Draw number of cards in this folder
        numberLabel.SetString(str);
        numberLabel.setOrigin(cardLabel.GetLocalBounds().width, 0);
        if (str.length() > 1) {
            numberLabel.setPosition(400.f, 14.f);
        }
        else {
            numberLabel.setPosition(414.f, 14.f);
        }
        if (nonempty.size() == 30) {
            numberLabel.SetFont(Font::Style::gradient_green);
        }
        else {
            numberLabel.SetFont(Font::Style::gradient);
        }

        surface.draw(numberLabel);

        numberLabel.SetFont(numberFont);
        numberLabel.SetString("/");
        numberLabel.setOrigin(0, 0);
        numberLabel.SetColor(sf::Color(200, 224, 248, 255));
        numberLabel.setPosition(418.f, 14.f);
        surface.draw(numberLabel);
        numberLabel.SetColor(sf::Color::White);
        if (nonempty.size() == 30) {
            numberLabel.SetFont(Font::Style::gradient_green);
        }
        else {
            numberLabel.SetFont(Font::Style::gradient);
        }

        // Draw max
        numberLabel.SetString(std::string("  30")); // will print "# / 30"
        numberLabel.setOrigin(0, 0);
        numberLabel.setPosition(414.f, 14.f);

        surface.draw(numberLabel);
    }

    // folder card count opens on FOLDER view mode only
    if (currViewMode == ViewMode::folder) {
        if (prevViewMode == currViewMode) { // camera pan finished
            scale = swoosh::ease::interpolate((float)frameElapsed * 8.f, 2.0f, folderCardCountBox.getScale().y);
        }
    }
    else {
        scale = swoosh::ease::interpolate((float)frameElapsed * 8.f, 0.0f, folderCardCountBox.getScale().y);
    }

    folderCardCountBox.setScale(2.0f, scale);

    DrawFolder(surface);
    DrawPool(surface);
}

void FolderEditScene::DrawFolder(sf::RenderTarget& surface) {
    cardDesc.setPosition(sf::Vector2f(26.f, 175.0f));
    scrollbar.setPosition(410.f, 60.f);
    cardHolder.setPosition(16.f, 32.f);
    element.setPosition(2.f * 28.f, 136.f);
    card.setPosition(96.f, 88.f);

    surface.draw(folderDock);
    surface.draw(cardHolder);

    // ScrollBar limits: Top to bottom screen position when selecting first and last card respectively
    float top = 50.0f; float bottom = 230.0f;
    float depth = ((float)folderView.lastCardOnScreen / (float)folderView.numOfCards) * bottom;
    scrollbar.setPosition(452.f, top + depth);

    surface.draw(scrollbar);

    // Move the card library iterator to the current highlighted card
    auto iter = folderCardSlots.begin();

    for (int j = 0; j < folderView.lastCardOnScreen; j++) {
        iter++;
    }

    // Now that we are at the viewing range, draw each card in the list
    for (int i = 0; i < folderView.maxCardsOnScreen && folderView.lastCardOnScreen + i < folderView.numOfCards; i++) {
        const Battle::Card& copy = iter->ViewCard();

        if (!iter->IsEmpty()) {
            float cardIconY = 66.0f + (32.f * i);
            cardIcon.setTexture(*GetIconForCard(copy.GetUUID()));
            cardIcon.setPosition(2.f * 104.f, cardIconY);
            surface.draw(cardIcon);

            cardLabel.SetColor(sf::Color::White);
            cardLabel.setPosition(2.f * 120.f, cardIconY + 4.0f);
            cardLabel.SetString(copy.GetShortName());
            surface.draw(cardLabel);

            int offset = (int)(copy.GetElement());
            element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
            element.setPosition(2.f * 183.f, cardIconY);
            surface.draw(element);

            cardLabel.setOrigin(0, 0);
            cardLabel.setPosition(2.f * 200.f, cardIconY + 4.0f);
            cardLabel.SetString(std::string() + copy.GetCode());
            surface.draw(cardLabel);

            //Draw MB
            mbPlaceholder.setPosition(2.f * 210.f, cardIconY + 2.0f);
            surface.draw(mbPlaceholder);
        }

        // Draw cursor
        if (folderView.lastCardOnScreen + i == folderView.currCardIndex) {
            auto y = swoosh::ease::interpolate((float)frameElapsed * 7.f, folderCursor.getPosition().y, 64.0f + (32.f * i));
            auto bounce = std::sin((float)totalTimeElapsed * 10.0f) * 5.0f;

            folderCursor.setPosition((2.f * 90.f) + bounce, y);
            surface.draw(folderCursor);

            if (!iter->IsEmpty()) {

                card.setTexture(*GetPreviewForCard(copy.GetUUID()));
                card.setScale((float)swoosh::ease::linear(cardRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f) * 2.0f, 2.0f);
                surface.draw(card);

                // This draws the currently highlighted card
                if (copy.GetDamage() > 0) {
                    cardLabel.SetColor(sf::Color::White);
                    cardLabel.SetString(std::to_string(copy.GetDamage()));
                    cardLabel.setOrigin(cardLabel.GetLocalBounds().width + cardLabel.GetLocalBounds().left, 0);
                    cardLabel.setPosition(2.f * 80.f, 142.f);

                    surface.draw(cardLabel);
                }

                cardLabel.setOrigin(0, 0);
                cardLabel.SetColor(sf::Color::Yellow);
                cardLabel.setPosition(2.f * 16.f, 142.f);
                cardLabel.SetString(std::string() + copy.GetCode());
                surface.draw(cardLabel);

                std::string formatted = FormatCardDesc(copy.GetDescription());
                cardDesc.SetString(formatted);
                surface.draw(cardDesc);

                int offset = (int)(copy.GetElement());
                element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
                element.setPosition(2.f * 32.f, 138.f);
                surface.draw(element);
            }
        }

        if (folderView.lastCardOnScreen + i == folderView.swapCardIndex && (int(totalTimeElapsed * 1000) % 2 == 0)) {
            auto y = 64.0f + (32.f * i);

            folderSwapCursor.setPosition((2.f * 95.f) + 2.0f, y);
            folderSwapCursor.setColor(sf::Color(255, 255, 255, 200));
            surface.draw(folderSwapCursor);
            folderSwapCursor.setColor(sf::Color::White);
        }

        iter++;
    }
}

void FolderEditScene::DrawPool(sf::RenderTarget& surface) {
    cardDesc.setPosition(sf::Vector2f(320.f + 480.f, 175.0f));
    packCardHolder.setPosition(310.f + 480.f, 35.f);
    element.setPosition(400.f + 2.f * 20.f + 480.f, 146.f);
    card.setPosition(389.f + 480.f, 93.f);

    surface.draw(packDock);
    surface.draw(packCardHolder);

    // ScrollBar limits: Top to bottom screen position when selecting first and last card respectively
    float top = 50.0f; float bottom = 230.0f;
    float depth = ((float)packView.lastCardOnScreen / (float)packView.numOfCards) * bottom;
    scrollbar.setPosition(292.f + 480.f, top + depth);

    surface.draw(scrollbar);

    if (packView.numOfCards == 0) return;

    // Move the card library iterator to the current highlighted card
    auto iter = poolCardBuckets.begin();

    for (int j = 0; j < packView.lastCardOnScreen; j++) {
        iter++;
    }

    // Now that we are at the viewing range, draw each card in the list
    for (int i = 0; i < packView.maxCardsOnScreen && packView.lastCardOnScreen + i < packView.numOfCards; i++) {
        int count = iter->GetCount();
        const Battle::Card& copy = iter->ViewCard();

        cardIcon.setTexture(*GetIconForCard(copy.GetUUID()));
        cardIcon.setPosition(16.f + 480.f, 65.0f + (32.f * i));
        surface.draw(cardIcon);

        cardLabel.SetColor(sf::Color::White);
        cardLabel.setPosition(49.f + 480.f, 69.0f + (32.f * i));
        cardLabel.SetString(copy.GetShortName());
        surface.draw(cardLabel);

        int offset = (int)(copy.GetElement());
        element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
        element.setPosition(182.0f + 480.f, 65.0f + (32.f * i));
        surface.draw(element);

        cardLabel.setOrigin(0, 0);
        cardLabel.setPosition(216.f + 480.f, 69.0f + (32.f * i));
        cardLabel.SetString(std::string() + copy.GetCode());
        surface.draw(cardLabel);

        //Draw MB
        mbPlaceholder.setPosition(236.f + 480.f, 67.0f + (32.f * i));
        surface.draw(mbPlaceholder);

        // Draw count in pack
        cardLabel.setOrigin(0, 0);
        cardLabel.setPosition(274.f + 480.f, 69.0f + (32.f * i));
        cardLabel.SetString(std::to_string(count));
        surface.draw(cardLabel);

        // Draw cursor
        if (packView.lastCardOnScreen + i == packView.currCardIndex) {
            auto y = swoosh::ease::interpolate((float)frameElapsed * 7.f, packCursor.getPosition().y, 64.0f + (32.f * i));
            auto bounce = std::sin((float)totalTimeElapsed * 10.0f) * 2.0f;

            packCursor.setPosition(bounce + 480.f + 2.f, y);
            surface.draw(packCursor);

            card.setTexture(*GetPreviewForCard(poolCardBuckets[packView.currCardIndex].ViewCard().GetUUID()));
            card.setTextureRect(sf::IntRect{ 0,0,56,48 });
            card.setScale((float)swoosh::ease::linear(cardRevealTimer.getElapsed().asSeconds(), 0.25f, 1.0f) * 2.0f, 2.0f);
            surface.draw(card);

            // This draws the currently highlighted card
            if (copy.GetDamage() > 0) {
                cardLabel.SetColor(sf::Color::White);
                cardLabel.SetString(std::to_string(copy.GetDamage()));
                cardLabel.setOrigin(cardLabel.GetLocalBounds().width + cardLabel.GetLocalBounds().left, 0);
                cardLabel.setPosition(2.f * (223.f) + 480.f, 145.f);

                surface.draw(cardLabel);
            }

            cardLabel.setOrigin(0, 0);
            cardLabel.SetColor(sf::Color::Yellow);
            cardLabel.setPosition(2.f * 167.f + 480.f, 145.f);
            cardLabel.SetString(std::string() + copy.GetCode());
            surface.draw(cardLabel);

            std::string formatted = FormatCardDesc(copy.GetDescription());
            cardDesc.SetString(formatted);
            surface.draw(cardDesc);

            int offset = (int)(copy.GetElement());
            element.setTextureRect(sf::IntRect(14 * offset, 0, 14, 14));
            element.setPosition(2.f * 179.f + 480.f, 142.f);
            surface.draw(element);
        }

        if (packView.lastCardOnScreen + i == packView.swapCardIndex && (int(totalTimeElapsed * 1000) % 2 == 0)) {
            auto y = 64.0f + (32.f * i);

            packSwapCursor.setPosition(485.f + 2.f + 2.f, y);
            packSwapCursor.setColor(sf::Color(255, 255, 255, 200));
            surface.draw(packSwapCursor);
            packSwapCursor.setColor(sf::Color::White);
        }

        iter++;
    }
}

void FolderEditScene::onEnd() {
}

void FolderEditScene::ExcludeFolderDataFromPool()
{
    Battle::Card mock; // will not be used
    for (auto& f : folderCardSlots) {
        auto iter = std::find_if(poolCardBuckets.begin(), poolCardBuckets.end(), [&f](PoolBucket& pack) { return pack.ViewCard() == f.ViewCard(); });
        if (iter != poolCardBuckets.end()) {
            iter->GetCard(mock);
        }
    }
}

void FolderEditScene::PlaceFolderDataIntoCardSlots()
{
    CardFolder::Iter iter = folder.Begin();

    while (iter != folder.End() && folderCardSlots.size() < 30) {
        auto slot = FolderSlot();
        slot.AddCard(Battle::Card(**iter));
        folderCardSlots.push_back(slot);
        iter++;
    }

    while (folderCardSlots.size() < 30) {
        folderCardSlots.push_back({});
    }
}

void FolderEditScene::PlaceLibraryDataIntoBuckets()
{
    CardLibrary::Iter iter = CHIPLIB.Begin();
    std::map<Battle::Card, bool, Battle::Card::Compare> visited; // visit table

    while (iter != CHIPLIB.End()) {
        if (visited.find(*iter) != visited.end()) {
            iter++;
            continue;
        }

        visited[(*iter)] = true;
        int count = CHIPLIB.GetCountOf(*iter);
        auto bucket = PoolBucket(count, Battle::Card(*iter));
        poolCardBuckets.push_back(bucket);
        iter++;
    }

    auto& packageManager = getController().CardPackageManager();
    std::string packageId = packageManager.FirstValidPackage();

    do {
      auto& meta = packageManager.FindPackageByID(packageId);

      for (auto& code : meta.GetCodes()) {
        Battle::Card::Properties props = meta.GetCardProperties();
        props.code = code;
        auto bucket = PoolBucket(5, Battle::Card(props));
        poolCardBuckets.push_back(bucket);
      }

      packageId = packageManager.GetPackageAfter(packageId);

    } while (packageId != packageManager.FirstValidPackage());
}

void FolderEditScene::WriteNewFolderData()
{
    folder = CardFolder();

    for (auto iter = folderCardSlots.begin(); iter != folderCardSlots.end(); iter++) {
        if ((*iter).IsEmpty()) continue;

        folder.AddCard((*iter).ViewCard());
    }
}

std::shared_ptr<sf::Texture> FolderEditScene::GetIconForCard(const std::string& uuid)
{
  auto& packageManager = getController().CardPackageManager();

  if (!packageManager.HasPackage(uuid))
    return nullptr;

  auto& meta = packageManager.FindPackageByID(uuid);
  return meta.GetIconTexture();
}
std::shared_ptr<sf::Texture> FolderEditScene::GetPreviewForCard(const std::string& uuid)
{
  auto& packageManager = getController().CardPackageManager();

  if (!packageManager.HasPackage(uuid))
    return nullptr;

  auto& meta = packageManager.FindPackageByID(uuid);
  return meta.GetPreviewTexture();
}

#ifdef __ANDROID__
void FolderEditScene::StartupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);

  rightSide.onTouch([]() {
    INPUTx.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([this](sf::Vector2i delta) {
    if (!releasedB) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_A);
    }
  });

  rightSide.onDrag([this](sf::Vector2i delta) {
    if (delta.x < -25 && !releasedB) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_B);
      INPUTx.VirtualKeyEvent(InputEvent::RELEASED_B);
      releasedB = true;
    }
  });
}

void FolderEditScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif
