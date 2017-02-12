//#include "Arduino.h"

//simulated Arduino
#include <stdio.h>
#include <iostream>
const int OUTPUT = 0;
const int LOW = 0;
const int HIGH = 1;
unsigned long time_elapsed = 0;
void pinMode(int pin, int mode) {
    printf("pinMode: %d - %d\n", pin, mode);
}
void digitalWrite(int pin, int value){
    printf("pin %d set to %d\n", pin, value);
}
void delay(int ms){
    printf("delayed for %d ms\n", ms);
    time_elapsed += ms;
}
unsigned long millis(){
    return time_elapsed;
}

//tile rotation
//rgb color support for all framebuffer
//scrolling spacing



class LEDMatrixTile
{
public:
    int x, y, width, height;
    bool *frameBuffer;
    char blendMode; //'n' = normal, 'a' = additive, 's' = subtractive
    virtual void render();
    virtual void init(int, int, bool*);
};

void LEDMatrixTile::init(int width, int height, bool *symbol)
{
    this->width = width;
    this->height = height;
    this->frameBuffer = new bool[width*height];
    for (int i = 0; i < width * height; i++){
        this->frameBuffer[i] = symbol[i];
    }
    this->blendMode = 'n';
}

void LEDMatrixTile::render()
{
    
}



class LEDMatrixActionTile: public LEDMatrixTile
{
private:
    bool *symbols[128];
    bool hasSymbol[128];
    int timer, actionsPtr, actionsEnd;
    unsigned long prevTime;
    int actionDelay;
    char transitionMode, transitionModeParam;
    bool *renderBuffer;
    void drawBuffer(bool*, int, int, int, int, int, int);
    void scrollBuffer(int, int);
    int scrollPtr;
    void clearBuffer(bool buffer[]);
public:
    int maxActions, maxTimeElapsed;
    char actions[1025];
    void init(int, int);
    void clearActions();
    void pushActions(char actions[]);
    void render();
    void addSymbol(char, bool*);
};

void LEDMatrixActionTile::init(int width, int height)
{
    this->maxActions = 1025;
    this->maxTimeElapsed = 1000;
    this->width = width;
    this->height = height;
    this->frameBuffer = new bool[width*height];
    this->renderBuffer = new bool[width*height];
    this->timer = 0;
    this->actionDelay = 1000;
    this->transitionMode = 's';
    this->transitionModeParam = '\0';
    this->actionsPtr = 0;
    this->actionsEnd = 0;
    this->clearBuffer(this->frameBuffer);
    this->clearBuffer(this->renderBuffer);
    this->prevTime = millis();
    this->scrollPtr = -99;
    this->blendMode = 'n';
}

void LEDMatrixActionTile::clearActions()
{
    this->actionsPtr = 0;
    this->actionsEnd = 0;
    this->clearBuffer(this->frameBuffer);
    this->scrollPtr = -99;
}

void LEDMatrixActionTile::drawBuffer(bool* source, int s_x, int s_y, int d_x, int d_y, int width, int height)
{
    for (int h = 0; h < height; h++){
        for (int w = 0; w < width; w++){
            this->frameBuffer[(h+d_y)*this->height+(w+d_x)] = source[(h+s_y)*height+(w+s_x)];
        }
    }
}

void LEDMatrixActionTile::scrollBuffer(int x, int y)
{
    for (int h = 0; h < this->height; h++){
        for (int w = 0; w < this->width; w++){
            int _h = h-y, _w = w-x;
            if (_h >= 0 && _h < this->height && _w >= 0 && _w < this->width){
                this->renderBuffer[h*this->height+w] = this->frameBuffer[_h*this->height+_w];
            }
        }
    }
    for (int h = 0; h < this->height; h++){
        for (int w = 0; w < this->width; w++){
            this->frameBuffer[h*this->height+w] = this->renderBuffer[h*this->height+w];
        }
    }
    this->clearBuffer(this->renderBuffer);
}

void LEDMatrixActionTile::clearBuffer(bool buffer[])
{
    for (int h = 0; h < height; h++){
        for (int w = 0; w < width; w++){
            buffer[h*this->height+w] = 0;
        }
    }
}

void LEDMatrixActionTile::render()
{
    if (this->timer <= 0){
        if (this->actionsEnd > 0){
            if (this->actionsEnd == this->actionsPtr){
                this->clearActions();
            }
            else{
                char current = this->actions[this->actionsPtr];
                if (this->actions[this->actionsPtr] != '['){
                    if (this->hasSymbol[current]){
                        if (this->transitionMode == 's'){
                            this->drawBuffer(this->symbols[current], 0, 0, 0, 0, this->width, this->height);
                            this->timer = this->actionDelay;
                            this->actionsPtr++;
                            this->scrollPtr = -99;
                        }
                        else if (this->transitionMode == 'm'){
                            if (this->transitionModeParam == 'l'){
                                if (this->scrollPtr == -99) this->scrollPtr = 0;
                                if (this->scrollPtr < this->width){
                                    this->scrollBuffer(-1, 0);
                                    this->drawBuffer(this->symbols[current], this->scrollPtr, 0, this->width-1, 0, 1, this->height);
                                    this->timer = this->actionDelay/this->width;
                                    this->scrollPtr++;
                                }
                                else {
                                    this->actionsPtr++;
                                    this->scrollPtr = -99;
                                }
                            }
                            else if (this->transitionModeParam == 'r'){
                                if (this->scrollPtr == -99) this->scrollPtr = this->width-1;
                                if (this->scrollPtr >= 0){
                                    this->scrollBuffer(1, 0);
                                    this->drawBuffer(this->symbols[current], this->scrollPtr, 0, 0, 0, 1, this->height);
                                    this->timer = this->actionDelay/this->width;
                                    this->scrollPtr--;
                                }
                                else {
                                    this->actionsPtr++;
                                    this->scrollPtr = -99;
                                }
                            }
                            else if (this->transitionModeParam == 'u'){
                                if (this->scrollPtr == -99) this->scrollPtr = 0;
                                if (this->scrollPtr < this->height){
                                    this->scrollBuffer(0, -1);
                                    this->drawBuffer(this->symbols[current], 0, this->scrollPtr, 0, this->height-1, this->width, 1);
                                    this->timer = this->actionDelay/this->height;
                                    this->scrollPtr++;
                                }
                                else {
                                    this->actionsPtr++;
                                    this->scrollPtr = -99;
                                }
                            }
                            else if (this->transitionModeParam == 'd'){
                                if (this->scrollPtr == -99) this->scrollPtr = this->height-1;
                                if (this->scrollPtr >= 0){
                                    this->scrollBuffer(0, 1);
                                    this->drawBuffer(this->symbols[current], 0, this->scrollPtr, 0, 0, this->width, 1);
                                    this->timer = this->actionDelay/this->height;
                                    this->scrollPtr--;
                                }
                                else {
                                    this->actionsPtr++;
                                    this->scrollPtr = -99;
                                }
                            }
                        }
                    }
                }
                else {
                    this->actionsPtr++;
                    current = this->actions[this->actionsPtr];
                    if (current == 'd'){
                        int duration = 0;
                        while (true){
                            this->actionsPtr++;
                            current = this->actions[this->actionsPtr];
                            if (current == ']') break;
                            duration = duration * 10 + (current - '0');
                        }
                        this->actionDelay = duration;
                        //printf("duration set to: %d\n", duration);
                        this->actionsPtr++;
                        current = this->actions[this->actionsPtr];
                    }
                    else if (current == 't'){
                        this->actionsPtr++;
                        this->transitionMode = this->actions[this->actionsPtr];
                        if (this->transitionMode == 'm'){
                            this->actionsPtr++;
                            this->transitionModeParam = this->actions[this->actionsPtr];
                        }
                        this->actionsPtr += 2;
                    }
                    else if (current == 'l'){
                        this->actionsPtr = 0;
                    }
                }
            }
        }
    }
    else {
        unsigned long timeElapsed = millis() - this->prevTime;
        if (timeElapsed > this->maxTimeElapsed) timeElapsed = this->maxTimeElapsed;
        else if (timeElapsed < 0) timeElapsed += 0xffffffff;
        this->timer -= timeElapsed;
        this->prevTime = millis();
    }
}

void LEDMatrixActionTile::addSymbol(char character, bool *symbol)
{
    this->symbols[character] = symbol;
    this->hasSymbol[character] = true;
}

void LEDMatrixActionTile::pushActions(char actions[])
{
    int length = 0;
    for (int i = 0; actions[i] != '\0'; i++){
        if (this->actionsEnd < this->maxActions){
            this->actions[this->actionsEnd] = actions[i];
            this->actionsEnd++;
        }
        length++;
    }
    //printf("pushActions: (");
    //std::cout << actions;
    //printf(") length:%d ptr:%d end:%d\n", length, this->actionsPtr, this->actionsEnd);
}


class LEDMatrix
{
private:
    int refreshDelay;
    bool *frameBuffer;
    int renderListPtr;
    void drawBuffer(bool*, char, int, int, int, int, int, int);
    void drawScreen(bool buffer[]);
    LEDMatrixTile *renderList[128];
public:
    int maxChildren;
    int width, height;
    int *outPorts, *inPorts;
    void init(int, int, int*, int*);
    void clearBuffer();
    void render();
    void setRefreshRate(int);
    void addChild(LEDMatrixTile&);
};

void LEDMatrix::init(int width, int height, int* outPorts, int* inPorts)
{
    this->maxChildren = 128;
    this->width = width;
    this->height = height;
    this->frameBuffer = new bool[width*height];
    this->outPorts = outPorts;
    this->inPorts = inPorts;
    this->refreshDelay = 10;
    for (int w = 0; w < width; w++){
        pinMode(outPorts[w], OUTPUT);
        digitalWrite(outPorts[w], HIGH);
    }
    for (int h = 0; h < height; h++){
        pinMode(inPorts[h], OUTPUT);
        digitalWrite(inPorts[h], LOW);
    }
    this->clearBuffer();
    this->renderListPtr = 0;
}

void LEDMatrix::setRefreshRate(int rate)
{
    this->refreshDelay = 1000/rate;
}

void LEDMatrix::drawBuffer(bool* source, char blendMode, int s_x, int s_y, int d_x, int d_y, int width, int height)
{
    for (int h = 0; h < height; h++){
        if (h+d_y < 0 || h+d_y >= this->height) continue;
        for (int w = 0; w < width; w++){
            if (w+d_x < 0 || w+d_x >= this->width) continue;
            if (blendMode == 'a') this->frameBuffer[(h+d_y)*this->height+(w+d_x)] |= source[(h+s_y)*height+(w+s_x)];
            else if (blendMode == 's') this->frameBuffer[(h+d_y)*this->height+(w+d_x)] &= source[(h+s_y)*height+(w+s_x)];
            else this->frameBuffer[(h+d_y)*this->height+(w+d_x)] = source[(h+s_y)*height+(w+s_x)];
        }
    }
}

void LEDMatrix::clearBuffer()
{
    for (int h = 0; h < this->height; h++){
        for (int w = 0; w < this->width; w++){
            this->frameBuffer[h*this->height+w] = 0;
        }
    }
}

void LEDMatrix::drawScreen(bool buffer[])
{
    for (int h = 0; h < this->height; h++){
        digitalWrite(inPorts[h], HIGH);
        for (int w = 0; w < this->width; w++){
            //printf("led %d %d \n", w, h);
            if (buffer[h*this->height+w]){
                //printf("turned on\n");
                digitalWrite(outPorts[w], LOW);
            }
        }
        delay(this->refreshDelay);
        digitalWrite(inPorts[h], LOW);
        for (int w = 0; w < this->width; w++){
            //printf("led %d %d \n", w, h);
            if (buffer[h*this->height+w]){
                //printf("turned off\n");
                digitalWrite(outPorts[w], HIGH);
            }
        }
    }
}

void LEDMatrix::addChild(LEDMatrixTile &child)
{
    if (this->renderListPtr < this->maxChildren){
        this->renderList[this->renderListPtr] = &child;
        this->renderListPtr++;
    }
}

void LEDMatrix::render()
{
    for (int i = 0; i < this->renderListPtr; i++){
        this->renderList[i]->render();
        this->drawBuffer(this->renderList[i]->frameBuffer, this->renderList[i]->blendMode, 0, 0, this->renderList[i]->x, this->renderList[i]->y, this->renderList[i]->width, this->renderList[i]->height);
    }
    this->drawScreen(this->frameBuffer);
}


LEDMatrixActionTile tile;
LEDMatrixTile tile2;
LEDMatrixTile tile3;
LEDMatrix stage;
//int inPorts[6] = {2,3,4,5,6,7};
//int outPorts[6] = {8,9,10,11,12,13};
int inPorts[] = {
    53,52,50,48,49,51,
    53,52,50,48,49,51,
    53,52,50,48,49,51
};
int outPorts[] = {
    24,25,26,27,28,29,
    14,15,16,17,18,19,
    32,33,34,35,36,37
};
bool s_a[] = {
    0,0,1,1,0,0,
    0,1,0,0,1,0,
    1,0,0,0,0,1,
    1,1,1,1,1,1,
    1,0,0,0,0,1,
    1,0,0,0,0,1
};

bool s_b[] = {
    1,0,0,0,0,0,
    1,0,0,0,0,0,
    1,1,1,1,1,1,
    1,0,0,0,0,1,
    1,0,0,0,0,1,
    1,1,1,1,1,1
};

bool s_c[] = {
    1,1,1,1,1,1,
    1,0,0,0,0,0,
    1,0,0,0,0,0,
    1,0,0,0,0,0,
    1,0,0,0,0,0,
    1,1,1,1,1,1
};

bool s_space[] = {
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0
};

bool s_4[] = {
    0,0,0,1,0,0,
    0,0,1,1,0,0,
    0,1,0,1,0,0,
    1,1,1,1,1,1,
    0,0,0,1,0,0,
    0,0,0,1,0,0
};

bool s_2[] = {
    1,1,1,1,1,1,
    0,0,0,0,0,1,
    1,1,1,1,1,1,
    1,0,0,0,0,0,
    1,0,0,0,0,0,
    1,1,1,1,1,1
};

bool s_0[] = {
    0,1,1,1,1,0,
    0,1,0,0,1,0,
    0,1,0,0,1,0,
    0,1,0,0,1,0,
    0,1,0,0,1,0,
    0,1,1,1,1,0
};

bool s_bar[] = {
    1,1,1,1,1,1,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0
};

bool s_spiner1[] = {
    1,1,0,0,1,1,
    1,1,0,0,1,1,
    0,0,1,1,0,0,
    0,0,1,1,0,0,
    1,1,0,0,1,1,
    1,1,0,0,1,1
};

bool s_spiner2[] = {
    0,0,1,1,0,0,
    0,0,1,1,0,0,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    0,0,1,1,0,0,
    0,0,1,1,0,0
};

bool s_block[] = {
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1
};

bool st_off[] = {
    0,1,0,
    1,0,1,
    0,1,0
};

//test
char actions[] = "[d1000][tml]abc 420 # [l]\0";
//moving bar
//char actions[] = "[d1000][tmu]u \0";
//spiner
//char actions[] = "[d500][ts]io\0";

int timer;
int timerCap;
int tile3vx;
int tile3vy;
unsigned long prevTime;

void setup()
{   
    stage.init(6, 6, outPorts, inPorts);
    stage.setRefreshRate(1000);
    
    tile.init(6,6);
    
    tile.addSymbol('a', s_a);
    tile.addSymbol('b', s_b);
    tile.addSymbol('c', s_c);
    tile.addSymbol(' ', s_space);
    tile.addSymbol('4', s_4);
    tile.addSymbol('2', s_2);
    tile.addSymbol('0', s_0);
    
    tile.addSymbol('u', s_bar);
    tile.addSymbol('i', s_spiner1);
    tile.addSymbol('o', s_spiner2);
    
    tile.addSymbol('#', s_block);
    
    tile.pushActions(actions);
    
    tile.x = 0;
    tile.y = 0;
    
    stage.addChild(tile);
    
    tile2.init(6, 6, s_block);

   // stage.addChild(tile2);
    
    tile3.init(3,3,st_off);
    
    stage.addChild(tile3);
    
    timerCap = 100;
    timer = timerCap;
    prevTime = millis();
}

void loop()
{
    if (timer <= 0){
        if (tile3.x == 0 && tile3.y == 0){
            tile3vx = 1;
            tile3vy = 0;
        }
        else if (tile3.x == 3 && tile3.y == 0){
            tile3vx = 0;
            tile3vy = 1;
        }
        else if (tile3.x == 3 && tile3.y == 3){
            tile3vx = -1;
            tile3vy = 0;
        }
        else if (tile3.x == 0 && tile3.y == 3){
            tile3vx = 0;
            tile3vy = -1;
        }
        tile3.x += tile3vx;
        tile3.y += tile3vy;
        timer = timerCap;
    }
    else {
        unsigned long timeElapsed = millis() - prevTime;
        if (timeElapsed < 0) timeElapsed += 0xffffffff;
        timer -= timeElapsed;
        prevTime = millis();
    }
    stage.clearBuffer();
    stage.render();
}


int main(){
    setup();
    loop();
    return 0;
}
