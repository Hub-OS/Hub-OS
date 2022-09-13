#include "bnStatusDirector.h"
#include "bnEntity.h"

std::vector<StatusBlocker> StatusBehaviorDirector::statusBlockers{
    StatusBlocker { Hit::freeze, Hit::stun },
    StatusBlocker { Hit::stun, Hit::freeze },
    StatusBlocker { Hit::bubble, Hit::freeze },
    StatusBlocker { Hit::confuse, Hit::freeze }
};

StatusBehaviorDirector::StatusBehaviorDirector(Entity& owner) : owner(owner) {
    currentStatus = {};
    bannedStatuses = { Hit::stun, Hit::freeze };
}

void StatusBehaviorDirector::AddStatus(Hit::Flags statusFlag, frame_time_t maxCooldown) {
    // Since we might use it twice, create it once. No need to repeat code.
    AppliedStatus statusToSet{ statusFlag, maxCooldown };
    AppliedStatus& statusToCheck = GetStatus(statusFlag);
    if (statusToCheck.statusFlag == statusFlag) {
        statusToCheck.remainingTime = maxCooldown;
    }
    else {
        currentStatus.push_back(statusToSet);
    }
}

void StatusBehaviorDirector::OnUpdate(double elapsed)
{
    frame_time_t _elapsed = from_seconds(elapsed);

    std::vector<Hit::Flags> cancelledStatuses;
    for (StatusBlocker blocker : statusBlockers) {
        if (GetStatus(blocker.blockingFlag).remainingTime > frames(0)) {
            cancelledStatuses.push_back(blocker.blockedFlag);
        }
    }

    for (Hit::Flags flags : cancelledStatuses) {
        GetStatus(flags).remainingTime = frames(0);
    }

    auto keyTestThunk = [this](const InputEvent& key) {
        bool pass = false;

        if (owner.InputState().Has(key)) {
            pass = true;
        }

        return pass;
    };

    bool anyKey = keyTestThunk(InputEvents::pressed_use_chip);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_down);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_up);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_left);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_move_right);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_shoot);
    anyKey = anyKey || keyTestThunk(InputEvents::pressed_special);

    for (AppliedStatus& statusToCheck : currentStatus) {
        if (statusToCheck.remainingTime > frames(0)) {
            if (anyKey && (statusToCheck.statusFlag == Hit::stun || statusToCheck.statusFlag == Hit::freeze || statusToCheck.statusFlag == Hit::bubble)) {
                statusToCheck.remainingTime -= frames(1);
            }
            statusToCheck.remainingTime -= _elapsed;
        }
    }
};

AppliedStatus& StatusBehaviorDirector::GetStatus(Hit::Flags flag) {
    // If the previous status is being called, grab it.
    for (AppliedStatus& status : currentStatus) {
        if (status.statusFlag == flag) {
            return status;
        }
    }

    currentStatus.push_back(AppliedStatus{ flag, frames(0) });
    return currentStatus.back();
};

void StatusBehaviorDirector::ClearStatus() {
    
    for (AppliedStatus& status : currentStatus) {
        status.remainingTime = frames(0);
        status.statusFlag = Hit::none;
    }
};

StatusBehaviorDirector::~StatusBehaviorDirector() {
};