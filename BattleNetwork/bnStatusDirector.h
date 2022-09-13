#pragma once

#include "bnLogger.h"
#include "bnHitProperties.h"
#include "bnFrameTimeUtils.h"
#include "bnInputEvent.h"

struct AppliedStatus {
    Hit::Flags statusFlag;
    frame_time_t remainingTime;
};

struct StatusBlocker {
    Hit::Flags blockingFlag; /*!<The flag that prevents the other from going through.*/
    Hit::Flags blockedFlag; /*!<The flag that is being prevented.*/
};

class Entity;

class StatusBehaviorDirector {
public:
    StatusBehaviorDirector(Entity& owner);
    virtual ~StatusBehaviorDirector();
    void AddStatus(Hit::Flags statusFlag, frame_time_t maxCooldown);
    void OnUpdate(double elapsed);
    AppliedStatus& GetStatus(Hit::Flags flag);
    void ClearStatus();
private:
    Entity& owner;
    static std::vector<StatusBlocker> statusBlockers;
    std::vector<Hit::Flags> bannedStatuses;
    std::vector<AppliedStatus> currentStatus;
    std::vector<InputEvent> lastFrameStates;
};