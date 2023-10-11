class Colored:
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

    @classmethod
    def warn(cls, msg):
        print(cls.YELLOW + msg + cls.ENDC)

def timed_range(n, name="Timed iteration"):
    import time
    for i in range(n):
        start = time.time()
        try:
            yield i
        finally:
            end = time.time()
            Colored.warn(f"{name}: round {i} took {round(end - start, 3)}s")
