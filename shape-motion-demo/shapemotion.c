/** \file shapemotion.c
 *  \brief 
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdlib.h>
#include "buzzer.h"


#define GREEN_LED BIT6

static int y = 0;

//velocity vectors for the ball
static int yy = 50;
static int x = 1;

static int score = 0;
static int highscore = 0;
static int lives = 1;
//static int song = 1;


u_int bgColor = COLOR_BLACK;     /**The background color */
int redrawScreen = 1;           /**Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**fence around playing field  */


//AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}}; /** player 1 */
AbRect rect20 = {abRectGetBounds, abRectCheck, {2,15}}; // player 2

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2-1, screenHeight/2-1}
};

Layer player = { /* white pable the player controls */
  (AbShape *)&rect20,
  {(screenWidth/2)-58, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0
};
 
Layer ball = {		/**< Layer has a white circle */
  (AbShape *)&circle4,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &player,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &ball
};

/** Moving Layer
 *  Linked list of layer references
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

MovLayer ml0 = {&ball, {2,2}, 0};
MovLayer m20 = {&player, {0,0}, &ml0};

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { 
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { 
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
        Vec2 pixelPos = {col, row};
        u_int color = bgColor;
        Layer *probeLayer;
        for (probeLayer = layers; probeLayer;probeLayer = probeLayer->next) { 
            if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
                color = probeLayer->color;
                break; 
            }     
            } 
        lcd_writeColor(color); 
      } 
    } 
  } 
}	  

u_char collide(Vec2* p, Region* r)
{
  if(p->axes[0] < (r->topLeft.axes[0])+5 || p->axes[0] > (r->botRight.axes[0])+5)
    return 0;
  if(p->axes[1] < (r->topLeft.axes[1])+5 || p->axes[1] > (r->botRight.axes[1])+5)
    return 0;

  return 1;
}

static char current_state = 0;

static char tmDown = 0;
void noiseMaker()
{
  tmDown = 5;
  buzzer_set_period(250);
}
#define QUARTER_NOTE 0x88b80
#define C1 3804
#define D1 3390
#define E1 3020
void play_GameOver(){
  
  long count = 0;
  long tempo = QUARTER_NOTE;

  buzzer_set_period(E1);
  while(++count < tempo){}
  count = 0; 
  buzzer_set_period(D1);
  while(++count < tempo){}
  count = 0;
  buzzer_set_period(C1);
  while(++count < tempo){}
}
void stop_Song(){
    buzzer_set_period(0);
}

//checks for button presses to move the player
void checkInput()
{

    unsigned char isS1 = (p2sw_read() & 1) ? 0 : 1;
    unsigned char isS2 = (p2sw_read() & 8) ? 0 : 1;
    if(isS1){
      y = -7;
    }
    else if(isS2){
      y = 7;
    }
    else{
      y = 0;
    }
    Vec2 newVelocity = {0, y};
    (&m20)->velocity = newVelocity;
}

//collision checker for the padle
void Collider(){
    MovLayer *playerLayer = &m20;
    Region playerBoundary;
    layerGetBounds((playerLayer->layer), &playerBoundary);


    Vec2 pos = (&ball)->pos;    //this 3 lines collide the ball with the player
    Vec2 iniVelocity = (&ml0)->velocity;
    Vec2 bounceVelocity = {x,yy};

    if(collide(&pos, &playerBoundary)){
      if((&iniVelocity)->axes[0] < 0 && (&iniVelocity)->axes[1] <= 0){
        x = 2;
        yy = -2;
      }
      else if ((&iniVelocity)->axes[0] < 0 && (&iniVelocity)->axes[1] > 0){
        x = 2;
        yy = 2;
      }

      (&ml0)->velocity = bounceVelocity;
      noiseMaker();
      scoreUpdater();
      scoreDrawer();
    }
     mlAdvance(&m20, &fieldFence);
  
}

char buffer[4];
char buffer2[4];
void scoreDrawer() //draws the score on the bottom part of the screen
{
  drawString5x7(50,150, "    ", COLOR_BLUE, COLOR_BLACK);
  drawString5x7(50,150, buffer, COLOR_BLUE, COLOR_BLACK);
  score+=10; //the counter adds 10 points each time the method is called in the collision

}
void scoreUpdater()
{
  itoa(score, buffer, 10);
  itoa(highscore, buffer2,10);
  if(score > highscore){
    highscore = score;
    highscore -=10;
  }
}



void onPlayingState(){
  checkInput();
  Collider();
  tmDown--;
  if(tmDown <= 0)
    buzzer_set_period(0);
  if(lives <= 0)
    current_state = 1;
 
  mlAdvance(&ml0, &fieldFence);
  redrawScreen = 1;
  //song = 1;
}

void onGameoverState(){
  //if(song > 0){
  //    play_GameOver();
  //}
  //song = 0;
  //stop_Song();
  drawString5x7(60,10, buffer2, COLOR_RED, COLOR_BLACK);
  
  drawString5x7(10,screenHeight/2, "Insert another coin", COLOR_RED, COLOR_BLACK);
  
  drawString5x7(35,35, "YOU DIED!", COLOR_RED, COLOR_BLACK);
  
  unsigned char isS1 = (p2sw_read() & 1) ? 0 : 1;
  unsigned char isS2 = (p2sw_read() & 2) ? 0 : 1;
  unsigned char isS3 = (p2sw_read() & 4) ? 0 : 1;
  unsigned char isS4 = (p2sw_read() & 8) ? 0 : 1;

  if(isS1 || isS2 || isS3 || isS4){ // press any button on second level to continue the game
    current_state = 0;
    lives = 1;   
    drawString5x7(60,10, "        ", COLOR_RED, COLOR_BLACK);  ///these 3 lines are used to delete the game over text
    drawString5x7(10,screenHeight/2, "                   ", COLOR_RED, COLOR_BLACK);
    drawString5x7(35,35, "             ", COLOR_RED, COLOR_BLACK);
    score = 0;
    highscore = 0;
  }


}

void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
            if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
        (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	      newPos.axes[axis] += (2*velocity);
            }	/**< if outside of fence */
	    else if(shapeBoundary.topLeft.axes[0] < (fence->topLeft.axes[1])){
	      shapeInit();
	      newPos.axes[0] = screenWidth/2;
	      newPos.axes[1] = screenHeight/2;
	      score = 50;
	      drawString5x7(50,10, "    ", COLOR_WHITE, COLOR_BLACK);
	      lives--; 
	    }
	      
    }
    ml->layer->posNext = newPos;
  }
}



/** main initialization of the game
 */
void main()
{
  P1DIR |= GREEN_LED;
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15); 

  shapeInit();
  buzzer_init();
  layerInit(&fieldLayer);
  layerDraw(&fieldLayer);

  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();
  or_sr(0x8);

  //loop for redrawing
  for(;;) { 
    while (!redrawScreen) { 
      P1OUT &= ~GREEN_LED;
      or_sr(0x10);
    }
    P1OUT |= GREEN_LED;
    redrawScreen = 0;
    movLayerDraw(&m20, &fieldLayer);
    scoreUpdater();
    drawString5x7(10,150, "score:", COLOR_WHITE, COLOR_BLACK);

    
  }
}





/** Watchdog timer interrupt handler*/
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;
  count ++;
  if (count == 10) {
    updateState(current_state);
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;
}
