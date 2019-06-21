#include <OrangutanAnalog.h>
#include <OrangutanBuzzer.h>
#include <OrangutanLCD.h>
#include <OrangutanLEDs.h>
#include <OrangutanMotors.h>
#include <OrangutanPushbuttons.h>
#include <Pololu3pi.h>
#include <PololuQTRSensors.h>
#include <avr/pgmspace.h>

Pololu3pi robot;
unsigned int sensors[5];  // an array to hold sensor values
int turn_threshold = 200;


// Introductory messages.  The "PROGMEM" identifier causes the data to
// go into program space.
const char welcome_line1[] PROGMEM = " Pololu";
const char welcome_line2[] PROGMEM = "3\xf7 Robot";
const char demo_name_line1[] PROGMEM = "Maze";
const char demo_name_line2[] PROGMEM = "solver";

// A couple of simple tunes, stored in program space.
const char welcome[] PROGMEM = ">g32>>c32";
const char go[] PROGMEM = "L16 cdegreg4";

// Data for generating the characters used in load_custom_characters
// and display_readings.  By reading levels[] starting at various
// offsets, we can generate all of the 7 extra characters needed for a
// bargraph.  This is also stored in program space.
const char levels[] PROGMEM = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000,
                               0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};

// This function loads custom characters into the LCD.  Up to 8
// characters can be loaded; we use them for 7 levels of a bar graph.
void load_custom_characters() {
    OrangutanLCD::loadCustomCharacter(levels + 0, 0);  // no offset, e.g. one bar
    OrangutanLCD::loadCustomCharacter(levels + 1, 1);  // two bars
    OrangutanLCD::loadCustomCharacter(levels + 2, 2);  // etc...
    OrangutanLCD::loadCustomCharacter(levels + 3, 3);
    OrangutanLCD::loadCustomCharacter(levels + 4, 4);
    OrangutanLCD::loadCustomCharacter(levels + 5, 5);
    OrangutanLCD::loadCustomCharacter(levels + 6, 6);
    OrangutanLCD::clear();  // the LCD must be cleared for the characters to take effect
}

// This function displays the sensor readings using a bar graph.
void display_readings(const unsigned int* calibrated_values) {
    unsigned char i;

    for (i = 0; i < 5; i++) {
        // Initialize the array of characters that we will use for the
        // graph.  Using the space, an extra copy of the one-bar
        // character, and character 255 (a full black box), we get 10
        // characters in the array.
        const char display_characters[10] = {' ', 0, 0, 1, 2, 3, 4, 5, 6, 255};

        // The variable c will have values from 0 to 9, since
        // calibrated values are in the range of 0 to 1000, and
        // 1000/101 is 9 with integer math.
        char c = display_characters[calibrated_values[i] / 101];

        // Display the bar graph character.
        OrangutanLCD::print(c);
    }
}

// Initializes the 3pi, displays a welcome message, calibrates, and
// plays the initial music.  This function is automatically called
// by the Arduino framework at the start of program execution.
void setup() {
    unsigned int counter;  // used as a simple timer

    // This must be called at the beginning of 3pi code, to set up the
    // sensors.  We use a value of 2000 for the timeout, which
    // corresponds to 2000*0.4 us = 0.8 ms on our 20 MHz processor.
    robot.init(2000);

    load_custom_characters();  // load the custom characters

    // Play welcome music and display a message
    OrangutanLCD::printFromProgramSpace(welcome_line1);
    OrangutanLCD::gotoXY(0, 1);
    OrangutanLCD::printFromProgramSpace(welcome_line2);
    OrangutanBuzzer::playFromProgramSpace(welcome);
    delay(1000);

    OrangutanLCD::clear();
    OrangutanLCD::printFromProgramSpace(demo_name_line1);
    OrangutanLCD::gotoXY(0, 1);
    OrangutanLCD::printFromProgramSpace(demo_name_line2);
    delay(1000);

    // Display battery voltage and wait for button press
    while (!OrangutanPushbuttons::isPressed(BUTTON_B)) {
        int bat = OrangutanAnalog::readBatteryMillivolts();

        OrangutanLCD::clear();
        OrangutanLCD::print(bat);
        OrangutanLCD::print("mV");
        OrangutanLCD::gotoXY(0, 1);
        OrangutanLCD::print("Press B");

        delay(100);
    }

    // Always wait for the button to be released so that 3pi doesn't
    // start moving until your hand is away from it.
    OrangutanPushbuttons::waitForRelease(BUTTON_B);
    delay(1000);

    // Auto-calibration: turn right and left while calibrating the
    // sensors.
    for (counter = 0; counter < 80; counter++) {
        if (counter < 20 || counter >= 60)
            OrangutanMotors::setSpeeds(40, -40);
        else
            OrangutanMotors::setSpeeds(-40, 40);

        // This function records a set of sensor readings and keeps
        // track of the minimum and maximum values encountered.  The
        // IR_EMITTERS_ON argument means that the IR LEDs will be
        // turned on during the reading, which is usually what you
        // want.
        robot.calibrateLineSensors(IR_EMITTERS_ON);

        // Since our counter runs to 80, the total delay will be
        // 80*20 = 1600 ms.
        delay(20);
    }
    OrangutanMotors::setSpeeds(0, 0);

    // Display calibrated values as a bar graph.
    while (!OrangutanPushbuttons::isPressed(BUTTON_B)) {
        // Read the sensor values and get the position measurement.
        unsigned int position = robot.readLine(sensors, IR_EMITTERS_ON);

        // Display the position measurement, which will go from 0
        // (when the leftmost sensor is over the line) to 4000 (when
        // the rightmost sensor is over the line) on the 3pi, along
        // with a bar graph of the sensor readings.  This allows you
        // to make sure the robot is ready to go.
        OrangutanLCD::clear();
        OrangutanLCD::print(position);
        OrangutanLCD::gotoXY(0, 1);
        display_readings(sensors);

        delay(100);
    }
    OrangutanPushbuttons::waitForRelease(BUTTON_B);

    OrangutanLCD::clear();
    OrangutanLCD::print("Go!");

    // Play music and wait for it to finish before we start driving.
    // OrangutanBuzzer::playFromProgramSpace(go);
    // while(OrangutanBuzzer::isPlaying());
    delay(250);
}


// This function, causes the 3pi to follow a segment of the maze until
// it detects an intersection, a dead end, or the finish.
long follow_segment(boolean solved, int duration, boolean final) {
    int last_proportional = 0;
    long integral = 0;

    int start_time = millis();
    int max_speed = 65;

    while (1) {
        // Normally, we will be following a line.  The code below is
        // similar to the 3pi-linefollower-pid example.

        // Get the position of the line.
        unsigned int position = robot.readLine(sensors, IR_EMITTERS_ON);

        // The "proportional" term should be 0 when we are on the line.
        int proportional = ((int) position) - 2000;

        // Compute the derivative (change) and integral (sum) of the
        // position.
        int derivative = proportional - last_proportional;
        integral += proportional;

        // Remember the last position.
        last_proportional = proportional;

        // Compute the difference between the two motor power settings,
        // m1 - m2.  If this is a positive number the robot will turn
        // to the left.  If it is a negative number, the robot will
        // turn to the right, and the magnitude of the number determines
        // the sharpness of the turn.
        int power_difference = proportional / 20 + integral / 10000 + derivative * 3 / 2;

        // Compute the actual motor settings.  We never set either motor
        // to a negative value.
        int maximum = solved ? max_speed : 60;  // the maximum speed

        if (solved && ((millis() - start_time) * maximum <= duration)) {
            int pos_power = power_difference > 0 ? power_difference : -power_difference;
            if (pos_power > 15) {
                maximum = 65;
            } else {
                maximum = 100;
            }
        }

        if (final) {
            int pos_power = power_difference > 0 ? power_difference : -power_difference;
            if (pos_power > 15) {
                maximum = 65;
            } else {
                maximum = 200;
            }
        }

        if (power_difference > maximum) power_difference = maximum;
        if (power_difference < -maximum) power_difference = -maximum;

        if (power_difference < 0)
            OrangutanMotors::setSpeeds(maximum + power_difference, maximum);
        else
            OrangutanMotors::setSpeeds(maximum, maximum - power_difference);

        // We use the inner three sensors (1, 2, and 3) for
        // determining whether there is a line straight ahead, and the
        // sensors 0 and 4 for detecting lines going to the left and
        // right.

        if (sensors[1] < turn_threshold && sensors[2] < turn_threshold &&
            sensors[3] < turn_threshold) {
            // There is no line visible ahead, and we didn't see any
            // intersection.  Must be a dead end.
            return (millis() - start_time) * maximum;
        } else if (sensors[0] > turn_threshold || sensors[4] > turn_threshold) {
            // Found an intersection.
            return (millis() - start_time) * maximum;
        }
    }
}


// Code to perform various types of turns according to the parameter dir,
// which should be 'L' (left), 'R' (right), 'S' (straight), or 'B' (back).
// The delays here had to be calibrated for the 3pi's motors.
void turn(unsigned char dir) {
    switch (dir) {
        case 'L': {
            // Turn left.
            OrangutanMotors::setSpeeds(-80, 80);
            delay(200);
        } break;
        case 'R': {
            // Turn right.
            OrangutanMotors::setSpeeds(80, -80);
            delay(200);
        } break;
        case 'B': {
            // Turn around.
            OrangutanMotors::setSpeeds(80, -80);
            delay(350);
        } break;
        case 'S':
            // Don't do anything!
            break;
    }
}


// The path variable will store the path that the robot has taken.  It
// is stored as an array of characters, each of which represents the
// turn that should be made at one intersection in the sequence:
//  'L' for left
//  'R' for right
//  'S' for straight (going straight through an intersection)
//  'B' for back (U-turn)
//
// Whenever the robot makes a U-turn, the path can be simplified by
// removing the dead end.  The follow_next_turn() function checks for
// this case every time it makes a turn, and it simplifies the path
// appropriately.
char path[100] = "";
int times[100] = {0};           // times it takes to travel each segment
unsigned char path_length = 0;  // the length of the path

// Displays the current path on the LCD, using two rows if necessary.
void display_path() {
    // Set the last character of the path to a 0 so that the print()
    // function can find the end of the string.  This is how strings
    // are normally terminated in C.
    path[path_length] = 0;
    times[path_length] = 0;

    OrangutanLCD::clear();
    OrangutanLCD::print(path);

    if (path_length > 8) {
        OrangutanLCD::gotoXY(0, 1);
        OrangutanLCD::print(path + 8);
    }
}

// HELPER: print string
void display_string(char* m) {
    OrangutanLCD::clear();
    OrangutanLCD::print(m);
}

// This function decides which way to turn during the learning phase of
// maze solving.  It uses the variables found_left, found_straight, and
// found_right, which indicate whether there is an exit in each of the
// three directions, applying the "left hand on the wall" strategy.
unsigned char select_turn(unsigned char found_left, unsigned char found_straight,
                          unsigned char found_right) {
    // Make a decision about how to turn.  The following code
    // implements a left-hand-on-the-wall strategy, where we always
    // turn as far to the left as possible.
    if (found_left)
        return 'L';
    else if (found_straight)
        return 'S';
    else if (found_right)
        return 'R';
    else
        return 'B';
}

void new_simplify_path() {
    while (path_length >= 3 && path[path_length - 2] == 'B') {
        if (path[path_length - 1] == 'L') {
            if (path[path_length - 3] == 'R')
                path[path_length - 3] = 'B';
            else if (path[path_length - 3] == 'L')
                path[path_length - 3] = 'S';
            else if (path[path_length - 3] == 'S')
                path[path_length - 3] = 'R';
        } else if (path[path_length - 1] == 'S') {
            if (path[path_length - 3] == 'L')
                path[path_length - 3] = 'R';
            else if (path[path_length - 3] == 'S')
                path[path_length - 3] = 'B';
        } else if (path[path_length - 1] == 'R') {
            if (path[path_length - 3] == 'L') path[path_length - 3] = 'B';
        }

        path_length -= 2;
    }
}

// This function comprises the body of the maze-solving program.  It is called
// repeatedly by the Arduino framework.
void loop() {
    while (1) {
        long follow_time = follow_segment(false, 0, false);

        // Drive straight a bit.  This helps us in case we entered the
        // intersection at an angle.
        // Note that we are slowing down - this prevents the robot
        // from tipping forward too much.
        OrangutanMotors::setSpeeds(50, 50);
        delay(50);

        // These variables record whether the robot has seen a line to the
        // left, straight ahead, and right, whil examining the current
        // intersection.
        unsigned char found_left = 0;
        unsigned char found_straight = 0;
        unsigned char found_right = 0;

        // Now read the sensors and check the intersection type.
        unsigned int sensors[5];
        robot.readLine(sensors, IR_EMITTERS_ON);

        // Check for left and right exits.
        if (sensors[0] > turn_threshold) found_left = 1;
        if (sensors[4] > turn_threshold) found_right = 1;

        // Drive straight a bit more - this is enough to line up our
        // wheels with the intersection.
        OrangutanMotors::setSpeeds(40, 40);
        delay(175);

        // Check for a straight exit.
        robot.readLine(sensors, IR_EMITTERS_ON);
        if (sensors[1] > turn_threshold || sensors[2] > turn_threshold ||
            sensors[3] > turn_threshold)
            found_straight = 1;

        // Check for the ending spot.
        // If all three middle sensors are on dark black, we have
        // solved the maze.
        if (sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600 && sensors[0] > 600 &&
            sensors[4] > 600)
            break;

        // Intersection identification is complete.
        // If the maze has been solved, we can follow the existing
        // path.  Otherwise, we need to learn the solution.
        unsigned char dir = select_turn(found_left, found_straight, found_right);

        // Make the turn indicated by the path.
        turn(dir);

        // Store the intersection in the path variable.
        path[path_length] = dir;
        times[path_length] = follow_time;
        path_length++;

        // You should check to make sure that the path_length does not
        // exceed the bounds of the array.  We'll ignore that in this
        // example.

        // Simplify the learned path.
        new_simplify_path();

        // Display the path on the LCD.
        display_path();
    }

    // Solved the maze!

    // Now enter an infinite loop - we can re-run the maze as many
    // times as we want to.
    while (1) {
        // Beep to show that we solved the maze.
        OrangutanMotors::setSpeeds(0, 0);
        OrangutanBuzzer::play(">>a32");

        // Wait for the user to press a button, while displaying
        // the solution.
        while (!OrangutanPushbuttons::isPressed(BUTTON_B)) {
            if (millis() % 2000 < 1000) {
                OrangutanLCD::clear();
                OrangutanLCD::print("Solved!");
                OrangutanLCD::gotoXY(0, 1);
                OrangutanLCD::print("Press B");
            } else
                display_path();
            delay(30);
        }


        while (OrangutanPushbuttons::isPressed(BUTTON_B))
            ;

        delay(250);

        // Re-run the maze.  It's not necessary to identify the
        // intersections, so this loop is really simple.
        int i;
        for (i = 0; i < path_length; i++) {
            int duration = times[i];
            follow_segment(true, duration * .3, false);

            // Drive straight while slowing down, as before.
            OrangutanMotors::setSpeeds(50, 50);
            delay(50);
            OrangutanMotors::setSpeeds(40, 40);
            delay(175);

            // Make a turn according to the instruction stored in
            // path[i].
            turn(path[i]);
        }

        // Follow the last segment up to the finish.
        follow_segment(true, 0, true);

        // Now we should be at the finish!  Restart the loop.
    }
}
