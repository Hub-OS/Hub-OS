#pragma once

#include "bnLogger.h"
#include "bnHitProperties.h"
#include "bnFrameTimeUtils.h"

struct AppliedStatus {
    Hit::Flags statusFlag;
    frame_time_t remainingTime;
};

struct StatusBlocker {
    Hit::Flags blockingFlag; /*!<The flag that prevents the other from going through.*/
    Hit::Flags blockedFlag; /*!<The flag that is being prevented.*/
};

class StatusBehaviorDirector {
public:
    StatusBehaviorDirector();
    virtual ~StatusBehaviorDirector();
    void AddStatus(Hit::Flags statusFlag, frame_time_t maxCooldown, bool deffer);
    void OnUpdate(double elapsed);
    AppliedStatus& GetStatus(bool isPrevious, Hit::Flags flag);
private:
    static std::vector<StatusBlocker> statusBlockers;
    std::vector<Hit::Flags> bannedStatuses;
    std::vector<AppliedStatus> currentStatus;
};
