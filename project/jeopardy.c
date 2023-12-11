#include <msp430.h>
#include <libTimer.h>
#include <stdio.h>
#include "lcdutils.h"
#include "lcddraw.h"

#define FG_COLOR COLOR_WHITE
#define BG_COLOR COLOR_BLACK

#define LED BIT6		/* note that bit zero req'd for display */
#define SWITCHES 15

typedef struct {
  char *questionLine1;
  char *questionLine2;
  char *questionLine3;
  char *questionLine4;
  char *questionLine5;
  char *answer0;
  char *answer1;
  char *answer2;
  char *answer3;
  enum {AnswerA = 0, AnswerB = 1, AnswerC = 2, AnswerD = 3} answer;
} question;

question q1 = {
  "What is the ",
  "capital of the",
  "United States?",
  "",
  "",
  "Washington, D.C.",
  "New York",
  "Los Angeles",
  "Chicago",
  AnswerA
};

question q2 = {
  "What is the ",
  "answer to life,",
  "the universe,",
  "and everything?",
  "",
  "43",
  "42",
  "44",
  "45",
  AnswerB
};

int redrawScreen = 1;
int switches = 0;
char current_position = 0;
int roundNum = 0;
int score = 0;
static int time = 10;
int current_question_index = 0;
question *current_question = &q1;

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;

  if (switches & SWITCHES) { 	/* a switch is depresssed */
    redrawScreen = 1;
    for (char swNum = 0; swNum < 4; swNum++) { /* respond to lowest button pressed */
      int swFlag = 1 << swNum;
      if (switches & swFlag) {
	current_position = swNum;
	break;
      }
    }
  }
}

void draw_round()
{
  char round_string[10];
  sprintf(round_string, "Round: %02d", roundNum);
  drawString5x7(5, 5, round_string, FG_COLOR, BG_COLOR);
}

void draw_score()
{
  char score_string[10];
  sprintf(score_string, "Score: %02d", score);
  drawString5x7(70, 5, score_string, FG_COLOR, BG_COLOR);
}
void draw_timer()
{
  char score_string[10];
  sprintf(score_string, "Time: %02d", time);
  drawString5x7(70, 15, score_string, FG_COLOR, BG_COLOR);
}

void draw_question(question question)
{
  drawString5x7(5, 40, question.questionLine1, FG_COLOR, BG_COLOR);
  drawString5x7(5, 50, question.questionLine2, FG_COLOR, BG_COLOR);
  drawString5x7(5, 60, question.questionLine3, FG_COLOR, BG_COLOR);
  drawString5x7(5, 70, question.questionLine4, FG_COLOR, BG_COLOR);
  drawString5x7(5, 80, question.questionLine5, FG_COLOR, BG_COLOR);
  drawString5x7(20, 100, question.answer0, FG_COLOR, BG_COLOR);
  drawString5x7(20, 110, question.answer1, FG_COLOR, BG_COLOR);
  drawString5x7(20, 120, question.answer2, FG_COLOR, BG_COLOR);
  drawString5x7(20, 130, question.answer3, FG_COLOR, BG_COLOR);
}

void draw_answer_key()
{
  drawString5x7(5, 100, "A:", COLOR_GREEN, BG_COLOR);
  drawString5x7(5, 110, "B:", COLOR_BLUE, BG_COLOR);
  drawString5x7(5, 120, "C:", COLOR_PURPLE, BG_COLOR);
  drawString5x7(5, 130, "D:", COLOR_RED, BG_COLOR);
}

void draw_buttons()
{
  drawString5x7(5, 150, "A", COLOR_GREEN, BG_COLOR);
  drawString5x7(45, 150, "B", COLOR_BLUE, BG_COLOR);
  drawString5x7(85, 150, "C", COLOR_PURPLE, BG_COLOR);
  drawString5x7(120, 150, "D", COLOR_RED, BG_COLOR);
}

void draw_answer_page(question question, char answer)
{
  char answer_string[10];
  sprintf(answer_string, "Answer: %c", answer);
  draw_question(question);
  drawString5x7(5, 110, answer_string, FG_COLOR, BG_COLOR);
}

void next_question(){
  switch(current_question_index){
  case 0:
    current_question_index++;
    current_question = &q2;
    break;
  case 1:
    current_question_index = 0;
    current_question = &q1;
    break;
  default:
    current_question_index = 0;
    current_question = &q1;
    break;
  }

  draw_question(*current_question);
}

void wdt_c_handler()
{
  static int sec2Count = 0;
  static int sec1Count = 0;
  if (sec2Count++ >= 125) {		/* 2/sec */
    sec2Count = 0;
    redrawScreen = 1;
  }
  if (sec1Count++ >= 250) {		/* 1/sec */
    sec1Count = 0;
    //timer--;
    //draw_timer();
    redrawScreen = 1;
  }
}

void main()
{

  P1DIR |= LED;     /**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);                 /**< GIE (enable interrupts) */

  clearScreen(BG_COLOR);

  draw_score();
  draw_round();
  draw_timer();
  draw_question(*current_question);
  draw_answer_key();
  draw_buttons();
  
  while (1) {           /* forever */
    if (redrawScreen) {
      redrawScreen = 0;
    }

    P1OUT &= ~LED;  /* led off */
    or_sr(0x10);    /**< CPU OFF */
    P1OUT |= LED;   /* led on */
  }
}

/* Switch on S2 */
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  int answer = 0;
  if (P2IFG & BIT0) {
    P2IFG &= ~BIT0;
    answer = 0;
  }
  if (P2IFG & BIT1) {
    P2IFG &= ~BIT1;
    answer = 1;
  }
  if (P2IFG & BIT2) {
    P2IFG &= ~BIT2;
    answer = 2;
  }
  if (P2IFG & BIT3) {
    P2IFG &= ~BIT3;
    answer = 3;
  }
  if (answer == (*current_question).answer) {
    roundNum++;
    score++;
    clearScreen(BG_COLOR);
    draw_score();
    draw_round();
    draw_timer();
    next_question();
    draw_answer_key();
    draw_buttons();
  }

}