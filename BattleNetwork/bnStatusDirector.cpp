#include "bnStatusDirector.h"

std::vector<StatusBlocker> StatusBehaviorDirector::statusBlockers{
    StatusBlocker { Hit::freeze, Hit::stun },
    StatusBlocker { Hit::stun, Hit::freeze }
};

StatusBehaviorDirector::StatusBehaviorDirector() {
    currentStatus = {};
    bannedStatuses = { Hit::stun, Hit::freeze };
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
    std::vector<Hit::Flags> cancelledStatuses;
    for (StatusBlocker blocker : statusBlockers) {
        if (GetStatus(false, blocker.blockingFlag).remainingTime > frames(0)) {
            cancelledStatuses.push_back(blocker.blockedFlag);
        }
    }

    for (Hit::Flags flags : cancelledStatuses) {
        GetStatus(false, flags).remainingTime = frames(0);
    }

    for (AppliedStatus& statusToCheck : currentStatus) {
        if (statusToCheck.remainingTime > frames(0)) {
            statusToCheck.remainingTime -= from_seconds(elapsed);
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

StatusBehaviorDirector::~StatusBehaviorDirector() {
}