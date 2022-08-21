#include "bnStatusDirector.h"
#include "bnInputManager.h"

std::vector<StatusBlocker> StatusBehaviorDirector::statusBlockers{
    StatusBlocker { Hit::freeze, Hit::stun },
    StatusBlocker { Hit::stun, Hit::freeze }
};

StatusBehaviorDirector::StatusBehaviorDirector() {
    currentStatus = {};
    bannedStatuses = { Hit::stun, Hit::freeze };
    mashHandler = InputHandle();
}

void StatusBehaviorDirector::AddStatus(Hit::Flags statusFlag, frame_time_t maxCooldown, bool deffer) {
    // Since we might use it twice, create it once. No need to repeat code.
    AppliedStatus statusToSet{ statusFlag, maxCooldown };
    AppliedStatus& statusToCheck = GetStatus(deffer, statusFlag);
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
        if (GetStatus(false, blocker.blockingFlag).remainingTime > frames(0)) {
            cancelledStatuses.push_back(blocker.blockedFlag);
        }
    }

    for (Hit::Flags flags : cancelledStatuses) {
        GetStatus(false, flags).remainingTime = frames(0);
    }


    auto keyTestThunk = [this](const InputEvent& key) {
        bool pass = false;

        if (mashHandler.Input().Has(key)) {
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
            if (anyKey && statusToCheck.statusFlag == (Hit::stun || Hit::freeze)) {
                statusToCheck.remainingTime -= frames(1);
            }
            statusToCheck.remainingTime -= _elapsed;
        }
    }
};

AppliedStatus& StatusBehaviorDirector::GetStatus(bool isPrevious, Hit::Flags flag) {
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