import enum

class ReplacementPolicy(enum.Enum):
    LRU = enum.auto()
    MRU = enum.auto()
    FIFO = enum.auto()
    RANDOM = enum.auto()

    
class Cache:
    def __init__(self):
        self.size = 16
