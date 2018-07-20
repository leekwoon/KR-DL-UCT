import os

def _project_dir():
    d = os.path.dirname
    return d(d(os.path.abspath(__file__)))

def _data_dir():
    return os.path.join(_project_dir(), "data")

__VERSION__ = "kr_dl_uct"

class TrainingConfig(object):
    LEARNING_RATE = 0.001
    BATCH_SIZE = 256
    USE_VIRTUAL_SIMULATION = True 

    DEVICE = "gpu:0"

    # for supervised learning, read only those dcl files
    REFERENCES = [
     #   "AyumuGAT1",
    ]

    MEMORY_SIZE = 1000000
    MINIMUM_SIZE_OF_MEMORY = 200000 

    UPDATE_CURRENT_MODEL_EVERY = 600 
    SYNC_EVERY = 60 

class ResourceConfig:
    """
    Config describing all of the directories and resources needed during running this project
    """
    PROJECT_DIR = _project_dir()
    DATA_DIR = _data_dir()
    # table
    TABLE_DIR = os.path.join(DATA_DIR, "table")
    # model
    MODEL_DIR = os.path.join(PROJECT_DIR, "model")
    MODEL_CHECKPOINT_DIR = os.path.join(MODEL_DIR, "checkpoints")
    MODEL_LOG_DIR = os.path.join(MODEL_DIR, "logs")
    # data 
    REFERENCE_PROGRAM_DATA_DIR = os.path.join(DATA_DIR, "kr_drl") # os.path.join(DATA_DIR, "reference_program")
    SELFPLAY_DATA_DIR = os.path.join(DATA_DIR, "selfplay_data_%s" % __VERSION__)
    EVALUATION_DATA_DIR = os.path.join(DATA_DIR, "evaluation_data_%s" % __VERSION__)

    # train where
    SERVER_ADDRESS = "" 
    # db
    DB_ADDRESS = ""
    MAIN_LOG_PATH = os.path.join(PROJECT_DIR, "main.log")

def create_directories():
    print(' [*] create directories ...')
    dirs = [
        ResourceConfig.PROJECT_DIR,
        ResourceConfig.DATA_DIR,
        ResourceConfig.TABLE_DIR,
        ResourceConfig.MODEL_DIR,
        ResourceConfig.MODEL_CHECKPOINT_DIR,
        ResourceConfig.MODEL_LOG_DIR,
        ResourceConfig.SELFPLAY_DATA_DIR,
        ResourceConfig.EVALUATION_DATA_DIR]
    for d in dirs:
        if not os.path.exists(d):
            os.makedirs(d)
create_directories() 

class NetworkConfig:
    INPUT_IMAGE_HEIGHT = 32
    INPUT_IMAGE_WIDTH = 32 

    OUTPUT_IMAGE_HEIGHT = 32 
    OUTPUT_IMAGE_WIDTH = 32

    NUM_RESIDUAL_FILTERS = 32
    NUM_RESIDUAL_BLOCKS = 9
    FILTER_SIZE = 3

class PlayGroundConfig:
    """GAME Configuration"""
    # Execution uncertainty is modeled by asymmetric gaussian noise
    # std of x: RAND * 0.5 
    # std of y: RAND * 2.0 
    RAND = 0.145 
    STONE_RADIUS = 0.145
    HOUSE_RADIUS = 1.83

    X_PLAYAREA_MIN = 0
    X_PLAYAREA_MAX = 4.75
    Y_PLAYAREA_MIN = 3.05
    Y_PLAYAREA_MAX = 3.05 + 8.23
    
    PLAYAREA_HEIGHT = X_PLAYAREA_MAX - X_PLAYAREA_MIN
    PLAYAREA_WIDTH = Y_PLAYAREA_MAX - Y_PLAYAREA_MIN
    
    TEE_X = 2.375
    TEE_Y = 4.88 

class PolicyConfig:   
    # Weak shot
    WEAK_X_MIN = 0.
    WEAK_X_MAX = 4.75 

    WEAK_Y_MIN = 2
    WEAK_Y_MAX = 9 
    
    # Strong shot
    STRONG_X_MIN = 0. 
    STRONG_X_MAX = 4.75 
    
    STRONG_Y_MIN = -6.
    STRONG_Y_MAX = -4. 
    
    WEAK_HEIGHT = WEAK_X_MAX - WEAK_X_MIN
    STRONG_HEIGHT = STRONG_X_MAX - STRONG_X_MIN
    WIDTH = (STRONG_Y_MAX - STRONG_Y_MIN) + (WEAK_Y_MAX - WEAK_Y_MIN)

class ValueConfig:
    N_SCORE = 17 # from -8 to 8

class Config:
    """
    """
    VERSION = __VERSION__

    resource = ResourceConfig
    train = TrainingConfig
    play_ground = PlayGroundConfig
    policy = PolicyConfig
    value = ValueConfig
    network = NetworkConfig



