#pragma once

#include "types.h"

namespace Poker {
  enum class ActionType : uint8_t {
    Fold,
    Check,
    Call,
    Raise
  };

  struct Action {
    ActionType type;
    Chips raise = 0;
  }

  enum ActionFlag : uint8_t {
    CanFold = 1 << 0,
    CanCheck = 1 << 1,
    CanCall = 1 << 2,
    CanRaise = 1 << 3
  };

  struct LegalActions {
    uint8_t mask = 0;

    Chips toCall = 0;
    Chips minRaiseTo = 0;
    Chips maxRaiseTo = 0;

    bool allows(ActionFlag flag) const noexcept {
      return (mask & flag) != 0;
    }
  }
}
