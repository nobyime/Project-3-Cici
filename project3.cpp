/**
* Author: Xiling Wang
* Assignment: Lunar Lander
* Date due: [03/14/2026]
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/Entity.h"

constexpr int SCREEN_WIDTH  = 800*1.5,
              SCREEN_HEIGHT = 450*1.5,
              FPS           = 120;

//constexpr char BG_COLOUR[]  = "#524368ff";

constexpr float FIXED_TIMESTEP = 1.0f/60.0f,
                GRAVITY        = 7.0f,
                UP_ACC         = 10.0f, // W
                X_ACC          = 12.0f, // A/D
                DOWN_ACC       = 6.0f, // S
                DRAG           = 0.15f,
                STOP_VX        = 1.0f,
                FUEL_MAX       = 100.0f,
                BURN_ACC_PS    = 3.2f,
                BURN_IDLE_PS   = 0.7f;

constexpr int   MAX_PLANES = 6;
constexpr float PLANE_TIMER = 3.0f;
constexpr float PLANE_SPD = 120.0f;

AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f,
      gTimeAccumulator = 0.0f;

Texture2D gBgTexture;

Entity *gHeli = nullptr;
float gFuel   = FUEL_MAX;
float gIsAcc  = false;
bool gCrashed = false;
bool gFailed  = false;
bool gWin     = false;
float gBlowTimer = 0.0f;
constexpr float BLOW_DURATION = 5.0f / 12.0f;

Entity* gPlanes = nullptr;
float gPlaneTimer = PLANE_TIMER;

void initialize();
void processInput();
void update();
void render();
void shutdown();

struct bldgRange {float x1, x2, y1, y2;};

bldgRange gForbidden[] = {
    {37.5, 112.5, 450, 675},
    {112.5, 187.5, 487.5, 675},
    {262.5, 337.5, 262.5, 675},
    {375, 450, 412.5, 675},
    {450, 637.5, 487.5, 675},
    {637.5, 712.5, 337.5, 675},
    {825, 900, 600, 675},
    {1087.5, 1162.5, 487.5, 675}
};
int gForbiddenCount = 8;

bldgRange gWinBldg[] = {
    {750, 825, 412.5, 675},
    {900, 1050, 525, 675}
};

void getHeliColl(float &l, float &r, float &t, float &b)
{
    Vector2 p = gHeli->getPosition();
    Vector2 c = gHeli->getColliderDimensions();
    l = p.x - c.x / 2.0f;
    r = p.x + c.x / 2.0f;
    t = p.y - c.y / 2.0f;
    b = p.y + c.y / 2.0f;
}

void getPlaneColl(float &l, float &r, float &t, float &b, int i)
{
    Vector2 p = gPlanes[i].getPosition();
    Vector2 c = gPlanes[i].getColliderDimensions();
    l = p.x - c.x / 2.0f;
    r = p.x + c.x / 2.0f;
    t = p.y - c.y / 2.0f;
    b = p.y + c.y / 2.0f;
}

bool hitBldg(const bldgRange &bldg)
{
    float l, r, t, b;
    getHeliColl(l, r, t, b);

    bool X = (r < bldg.x1) || (l > bldg.x2);
    bool Y = (b < bldg.y1) || (t > bldg.y2);
    return !(X || Y);
}

bool hitPlane(int i)
{
    float pl, pr, pt, pb;
    getPlaneColl(pl, pr, pt, pb, i);

    float l, r, t, b;
    getHeliColl(l, r, t, b);

    bool X = (r < pl) || (l > pr);
    bool Y = (b < pt) || (t > pb);
    return !(X || Y);
}

void newPlane()
{
    for (int i = 0; i < MAX_PLANES; i++)
    {
        if(!gPlanes[i].isActive())
        {
            float y = (float)GetRandomValue(100, 180);
            gPlanes[i].activate();
            gPlanes[i].setAngle(0.0f);
            gPlanes[i].setPosition({SCREEN_WIDTH + 40.0f, y});
            gPlanes[i].setVelocity({-PLANE_SPD, 0.0f});
            gPlanes[i].setAcceleration({0.0f, 0.0f});
            return;
        }
    }
}

void restart()
{
    gFuel = FUEL_MAX;
    gCrashed = false;
    gFailed  = false;
    gWin     = false;
    gBlowTimer = 0.0f;

    delete gHeli;

    std::map<Direction, std::vector<int>> heliAtlas = {
        {STEADY,     { 0,  1,  2,  3,  4}},
        {UP,         { 5,  6,  7,  8,  9}},
        {DOWN,       {10, 11, 12, 13, 14}},
        {RIGHT,      {15, 16, 17, 18, 19}},
        {LEFT,       {20, 21, 22, 23, 24}},
        {UP_RIGHT,   {25, 26, 27, 28, 29}},
        {UP_LEFT,    {30, 31, 32, 33, 34}},
        {DOWN_RIGHT, {35, 36, 37, 38, 39}},
        {DOWN_LEFT,  {40, 41, 42, 43, 44}},
        {BLOW,       {45, 46, 47, 48, 49}},
        {BURNING,    {50, 51, 52, 53, 54}},
    };

    gHeli = new Entity(
        {-20.0f, 60.0f},
        { 64.0f, 64.0f},
        "images/helicopter.png",
        ATLAS,
        {5, 11},
        heliAtlas
    );

    gHeli -> setFrameSpeed(12);
    gHeli -> setColliderDimensions({55.0f, 55.0f});
    gHeli -> setVelocity({0.0f, 0.0f});
    gHeli -> setAcceleration({0.0f, GRAVITY});
    gHeli -> setDirection(STEADY);

    for (int i = 0; i < MAX_PLANES; i++)
    {
        gPlanes[i].setTexture("images/plane.png");
        gPlanes[i].setAngle(0.0f);
        gPlanes[i].setScale({64.0f, 64.0f});
        gPlanes[i].setColliderDimensions({55.0f, 20.0f});
        gPlanes[i].setPosition({SCREEN_WIDTH + 100.0f, 0.0f});
        gPlanes[i].setVelocity({0.0f, 0.0f});
        gPlanes[i].setAcceleration({0.0f, 0.0f});
        gPlanes[i].deactivate();
    }

    gPlaneTimer = PLANE_TIMER;
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Helicopter BOOM");

    gBgTexture = LoadTexture("images/bg.png");

    std::map<Direction, std::vector<int>> heliAtlas = {
        {STEADY,     { 0,  1,  2,  3,  4}},
        {UP,         { 5,  6,  7,  8,  9}},
        {DOWN,       {10, 11, 12, 13, 14}},
        {RIGHT,      {15, 16, 17, 18, 19}},
        {LEFT,       {20, 21, 22, 23, 24}},
        {UP_RIGHT,   {25, 26, 27, 28, 29}},
        {UP_LEFT,    {30, 31, 32, 33, 34}},
        {DOWN_RIGHT, {35, 36, 37, 38, 39}},
        {DOWN_LEFT,  {40, 41, 42, 43, 44}},
        {BLOW,       {45, 46, 47, 48, 49}},
        {BURNING,    {50, 51, 52, 53, 54}},
    };

    gHeli = new Entity(
        {-20.0f, 60.0f},
        { 64.0f, 64.0f},
        "images/helicopter.png",
        ATLAS,
        {5, 11},
        heliAtlas
    );

    gHeli -> setFrameSpeed(12);
    gHeli -> setColliderDimensions({55.0f, 55.0f});
    gHeli -> setVelocity({0.0f, 0.0f});
    gHeli -> setAcceleration({0.0f, GRAVITY});
    gHeli -> setDirection(STEADY);

    gPlanes = new Entity[MAX_PLANES];

    for (int i = 0; i < MAX_PLANES; i++)
    {
        gPlanes[i].setTexture("images/plane.png");
        gPlanes[i].setAngle(0.0f);
        gPlanes[i].setScale({64.0f, 64.0f});
        gPlanes[i].setColliderDimensions({55.0f, 20.0f});
        gPlanes[i].setPosition({SCREEN_WIDTH + 100.0f, 0.0f});
        gPlanes[i].setVelocity({0.0f, 0.0f});
        gPlanes[i].setAcceleration({0.0f, 0.0f});
        gPlanes[i].deactivate();
    }

    SetTargetFPS(FPS);
}

void processInput()
{
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
    
    if(IsKeyPressed(KEY_R)){
        restart();
        return;
    }

    if(gCrashed || gFailed || gWin) {gIsAcc = false; return;};

    Vector2 v = gHeli -> getVelocity();

    float ax = 0.0f;
    float ay = GRAVITY;

    bool canFly = (gFuel > 0.0f);

    bool L = canFly && (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
    bool R = canFly && (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D));
    bool U = canFly && (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
    bool D = canFly &&  (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S));

    if      (L && !R) ax = -X_ACC;
    else if (R && !L) ax =  X_ACC;
    else              ax = -DRAG * v.x;

    ay = GRAVITY;
    if (U) ay -= UP_ACC;
    if (D) ay += DOWN_ACC;

    gHeli -> setAcceleration({ax, ay});

    Direction dir = STEADY;
    const float e = 0.09f;

    if (!(L || R || U || D))
    {
        dir = STEADY;
    }
    else
    {
        bool moveRight = v.x > e;
        bool moveLeft  = v.x < -e;
        bool moveDown  = v.y > e;
        bool moveUp    = v.y < -e;

        if      (moveUp && moveRight)   dir = UP_RIGHT;
        else if (moveUp && moveLeft)    dir = UP_LEFT;
        else if (moveDown && moveRight) dir = DOWN_RIGHT;
        else if (moveDown && moveLeft)  dir = DOWN_LEFT;
        else if (moveUp)                dir = UP;
        else if (moveDown)              dir = DOWN;
        else if (moveRight)             dir = RIGHT;
        else if (moveLeft)              dir = LEFT;
        else                            dir = STEADY;
    }
    
    gHeli->setDirection(dir);
    gIsAcc = canFly && (L || R || U || D);
}

void update()
{
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Fixed timestep
    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        Vector2 a = gHeli->getAcceleration();

        if (!(gFailed || gWin))
        {
            gPlaneTimer -= FIXED_TIMESTEP;
            if (gPlaneTimer <= 0.0f)
            {
                newPlane();
                gPlaneTimer += PLANE_TIMER;
            }
        }

        for (int i = 0; i < MAX_PLANES; i++)
        {
            if (!gPlanes[i].isActive()) continue;

            gPlanes[i].update(FIXED_TIMESTEP, nullptr, 0);

            Vector2 p = gPlanes[i].getPosition();
            Vector2 s = gPlanes[i].getScale();
            if (p.x + (s.x / 2.0f) < 0.0f)
            {
                gPlanes[i].deactivate();
                continue;
            }

            if (!gFailed && !gWin && hitPlane(i))
            {
                gPlanes[i].setVelocity({0.0f, 0.0f});
                gPlanes[i].setAcceleration({0.0f, 0.0f});

                gFailed = true;
                gCrashed = true;
                gBlowTimer = BLOW_DURATION;

                gHeli->setDirection(BLOW);
                gHeli->setVelocity({0.0f, 0.0f});
                gHeli->setAcceleration({0.0f, GRAVITY});

                break;
            }
        }

        if(gCrashed)
        {
            if(gBlowTimer <= 0.0f)
            {
                gHeli->setDirection(BURNING);
            }
            else{
                gHeli->setDirection(BLOW);
            }
            gHeli->setAcceleration({0.0f, GRAVITY});
        }

        if (!(gCrashed || gFailed || gWin) && gFuel > 0.0f)
        {
            float burn = gIsAcc ? BURN_ACC_PS : BURN_IDLE_PS;
            gFuel -= burn * FIXED_TIMESTEP;
            if (gFuel < 0.0f) gFuel = 0.0f;
        }

        if(gFuel <= 0.0f)
        {
            Vector2 v = gHeli->getVelocity();
            float ax = -DRAG * v.x;
            if(fabs(v.x) < STOP_VX) ax = 0.0f;
            gHeli->setAcceleration({ax,GRAVITY});
        }
        gHeli->update(FIXED_TIMESTEP, nullptr, 0);

        Vector2 p = gHeli->getPosition();
        Vector2 s = gHeli->getScale();

        bool offLeft   = (p.x + s.x / 2.0f) < 0.0f;
        bool offRight  = (p.x - s.x / 2.0f) > SCREEN_WIDTH;
        bool offTop    = (p.y + s.y / 2.0f) < 0.0f;
        bool offBottom = (p.y - s.y / 2.0f) > SCREEN_HEIGHT;

        if (!gCrashed && (offLeft || offRight || offTop || offBottom))
        {
            gCrashed = true;
            gFailed = true;

            gBlowTimer = BLOW_DURATION;
            gHeli->setDirection(BLOW);

            gHeli->setVelocity({0.0f, 0.0f});
            gHeli->setAcceleration({0.0f, GRAVITY});

            gFuel = 0;
        }

        if (gFailed || gWin)
        {
            if (gFailed)
            {
                gBlowTimer -= FIXED_TIMESTEP;
                if (gBlowTimer <= 0.0f)
                {
                    gHeli->setDirection(BURNING);
                    gHeli->setFrameSpeed(3);
                }
                else
                {
                    gHeli->setDirection(BLOW);
                    gHeli->setFrameSpeed(6); 
                }
            }
            gHeli->update(FIXED_TIMESTEP, nullptr, 0);
            deltaTime -= FIXED_TIMESTEP;
            continue;
        }

        for (int i = 0; i < gForbiddenCount; i++)
        {
            if (hitBldg(gForbidden[i]))
            {
                gFailed = true;
                gCrashed = true;
                gBlowTimer = BLOW_DURATION;

                gHeli->setDirection(BLOW);
                gHeli->setVelocity({0.0f, 0.0f});
                gHeli->setAcceleration({0.0f, GRAVITY});
                break;
            }
        }

        for (int i = 0; i < 2; i++)
        {
            float l, r, t, b;
            getHeliColl(l, r, t, b);

            float topY = gWinBldg[i].y1;

            bool approachTop = fabs(b - topY) <= 3.0f;
            bool xOnRoof   = (r > gWinBldg[i].x1) && (l < gWinBldg[i].x2);

            float varX = 0.0f;
            if (i == 0) varX = 20.0f;
            if (i == 1) varX = 40.0f;

            if (approachTop && xOnRoof)
            {
                Vector2 p = gHeli->getPosition();
                Vector2 c = gHeli->getColliderDimensions();

                float topX = (gWinBldg[i].x1 + gWinBldg[i].x2) / 2.0f;
                bool isCenter = (p.x >= topX - varX) && (p.x <= topX + varX);

                if (isCenter)
                {
                    gWin = true;

                    gHeli->setVelocity({0.0f, 0.0f});
                    gHeli->setAcceleration({0.0f, 0.0f});
                    gHeli->setDirection(STEADY);
                    gHeli->freezeAnimationOnFirstFrame();

                    p.y = topY - c.y / 2.0f;
                    gHeli->setPosition(p);
                }
                else
                {
                    gFailed  = true;
                    gCrashed = true;
                    gBlowTimer = BLOW_DURATION;

                    gHeli->setDirection(BLOW);
                    gHeli->setVelocity({0.0f, 0.0f});
                    gHeli->setAcceleration({0.0f, GRAVITY});
                }

                break;
            }
        }
        deltaTime -= FIXED_TIMESTEP;
    }
    
    gTimeAccumulator = deltaTime;
}

void render()
{
    BeginDrawing();

    Rectangle textureAreaBg = {
        0.0f, 0.0f,
        static_cast<float>(gBgTexture.width),
        static_cast<float>(gBgTexture.height)
    };
    
    Rectangle destinationAreaBg = {
        0.0f, 0.0f,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT)
    };
    
    Vector2 originBg = {0.0f, 0.0f};

    DrawTexturePro(
        gBgTexture,
        textureAreaBg,
        destinationAreaBg,
        originBg,
        0.0f,
        WHITE
    );

    for (int i = 0; i < MAX_PLANES; i++)
    {
        if(gPlanes[i].isActive()) gPlanes[i].render();
    }

    gHeli->render();

    DrawText(TextFormat("Fuel: %.1f%%", (float)(100.0f * gFuel / FUEL_MAX)), 12, 12, 20, WHITE);
    if (gFailed)
    {
        DrawText("Mission FAILED", SCREEN_WIDTH/2 - 167, SCREEN_HEIGHT/2 - 80, 40, MAROON);
        DrawText("Press R to restart", SCREEN_WIDTH/2 - 117, SCREEN_HEIGHT/2 - 30, 24, BLACK);
    }
    if (gWin && (gHeli->getPosition()).x < 825)
    {
        DrawText("PERFECT Landing!", SCREEN_WIDTH/2 - 175, SCREEN_HEIGHT/2 - 80, 40, GOLD);
        DrawText("Press R to restart", SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 - 30, 24, BLACK);
    }
    else if (gWin && (gHeli->getPosition()).x > 900)
    {
        DrawText("GOOD Landing!", SCREEN_WIDTH/2 - 137, SCREEN_HEIGHT/2 - 80, 40, LIGHTGRAY);
        DrawText("Press R to restart", SCREEN_WIDTH/2 - 112, SCREEN_HEIGHT/2 - 30, 24, BLACK);
    }

    EndDrawing();
}

void shutdown()
{
    CloseWindow();
    UnloadTexture(gBgTexture);
    delete[] gPlanes;
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}