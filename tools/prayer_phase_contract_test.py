from enum import Enum


class Phase(Enum):
    EQAMA = 1
    SLEEP = 2
    AZKAR = 3


def prayer_phase(since, eqama, sleep):
    """Contract model for the production phase boundaries."""
    if since == eqama:
        return Phase.EQAMA
    if sleep > 0 and since > eqama and since <= eqama + sleep:
        return Phase.SLEEP
    if sleep > 0 and since == eqama + sleep + 1:
        return Phase.AZKAR
    return None


assert prayer_phase(10, 10, 1) is Phase.EQAMA
assert prayer_phase(11, 10, 1) is Phase.SLEEP
assert prayer_phase(12, 10, 1) is Phase.AZKAR
assert prayer_phase(18, 10, 8) is Phase.SLEEP
assert prayer_phase(19, 10, 8) is Phase.AZKAR
