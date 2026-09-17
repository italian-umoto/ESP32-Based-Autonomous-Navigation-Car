#define STATE_TRANSITION_PIN 5


typedef enum States {
  STATE1, STATE2
} State;
State curr_state = STATE1;

bool button_pressed_last_loop = false;


void setup() {
  // put your setup code here, to run once:
  
}

void loop() {
  bool is_button_clicked = false;
  

  // put your main code here, to run repeatedly:
  switch (curr_state) {
    case STATE1:

      break;
    case STATE2:
      break;
  }
}
