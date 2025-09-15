enum GameState
{
	TITLE_SCREEN = 0,
	ROLLING_DEMO = 1,
    GAME_START = 2,
    GAME_OVER = 3,
    HIGH_SCORE = 4
};

enum GameStages
{
    STAGE_LASANGELES = 0,
    STAGE_LASVEGAS = 1,
    STAGE_HOUSTON = 2,
    STAGE_STLOUIS = 3,
    STAGE_CHICAGO = 4,
    STAGE_NEWYORK = 5
};

enum GameDifficulty
{
    FIVEHUNDEDCC = 0,
    SEVENFIFTYCC = 1,
    TWELVEHUNDREDCC = 2
};

extern UBYTE game_stage;
extern UBYTE game_state;
extern UBYTE game_difficulty;

void InitializeGame();
void NewGame(UBYTE difficulty);
void GameLoop();
void SetBackGroundColor(UWORD color);