/*  
 * ____                     _      ______ _____    _____
  / __ \                   | |    |  ____|  __ \  |  __ \               
 | |  | |_ __   ___ _ __   | |    | |__  | |  | | | |__) |__ _  ___ ___ 
 | |  | | '_ \ / _ \ '_ \  | |    |  __| | |  | | |  _  // _` |/ __/ _ \
 | |__| | |_) |  __/ | | | | |____| |____| |__| | | | \ \ (_| | (_|  __/
  \____/| .__/ \___|_| |_| |______|______|_____/  |_|  \_\__,_|\___\___|
        | |                                                             
        |_|          
 Open LED Race
 An minimalist cars race for LED strip  
  
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 by gbarbarov@singulardevices.com  for Arduino day Seville 2019 

 Code made dirty and fast, next improvements in: 
 https://github.com/gbarbarov/led-race
 https://www.hackster.io/gbarbarov/open-led-race-a0331a
 https://twitter.com/openledrace
*/
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
//#include <NewTone.h>
//#include <NeoPixelBus.h>
//#include <NeoPixelAnimator.h>

#define MAX_LEDS 300    // MAX LEDs actives on strip
#define PIN_LED_STRIP A0 // R 500 ohms to DI pin for WS2812 and WS2813, for WS2813 BI pin of first LED to GND  ,  CAP 1000 uF to VCC 5v/GND,power supplie 5V 2A
#define PIN_PLAYER1 7   // switch player 1 to PIN and GND
#define PIN_PLAYER2 6   // switch player 2 to PIN and GND
#define PIN_AUDIO 3     // through CAP 2uf to speaker 8 ohms

#define COLOR_PLAYER1 track.Color(0, 255, 0) // green - color p1
#define COLOR_PLAYER2 track.Color(255, 0, 0) // red - color p2

#define DEF_GRAVITY 127

int TOTAL_LEDS_STRIP = MAX_LEDS; // total amount of leds on track
int TBEEP = 3;                   // # of times a beep is played
int DELAY_MS = 5;                // delay to refresh status and strip in ms
int WIN_MUSIC[] = {
    2637, 2637, 0, 2637,
    0, 2093, 2637, 0,
    3136};

byte gravity_map[MAX_LEDS]; // setup stores the (hardcoded) value 127 at every pixel of the strip

float speed_player1 = 0;
float speed_player2 = 0;
float dist1 = 0;
float dist2 = 0;

byte laps_player1 = 0;
byte laps_player2 = 0;

byte leader = 0;
byte laps_max = 5; //total laps race

float ACCELERATION = 0.2; // acceleration constant
float FRICTION = 0.015;   // friction constant
float GRAVITY = 0.003;    // gravity constant

byte flag_sw1 = 0;
byte flag_sw2 = 0;
byte draworder = 0;

unsigned long timestamp = 0;

Adafruit_NeoPixel track = Adafruit_NeoPixel(MAX_LEDS, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);

void winner_fx()
{
    int msize = sizeof(WIN_MUSIC) / sizeof(int);

    for (int note = 0; note < msize; note++)
    {
        tone(PIN_AUDIO, WIN_MUSIC[note], 200);
        delay(230);
        noTone(PIN_AUDIO);
    }
};

void reset_race_data()
{
    laps_player1 = 0;
    laps_player2 = 0;
    dist1 = 0;
    dist2 = 0;
    speed_player1 = 0;
    speed_player2 = 0;
    timestamp = 0;
}

void start_race()
{
    Serial.println("start race");

    for (int i = 0; i < TOTAL_LEDS_STRIP; i++)
    {
        track.setPixelColor(i, track.Color(0, 0, 0));
    };

    track.show();
    delay(1000);
    track.setPixelColor(12, track.Color(255, 0, 0));
    track.setPixelColor(11, track.Color(255, 0, 0));
    track.show();
    tone(PIN_AUDIO, 400);
    delay(2000);
    noTone(PIN_AUDIO);
    for (int i = 0; i < 12; i++)
    {
        track.setPixelColor(i, track.Color(0, 0, 0));
    };

    track.show();
    track.setPixelColor(12, track.Color(0, 0, 0));
    track.setPixelColor(11, track.Color(0, 0, 0));
    track.setPixelColor(10, track.Color(255, 255, 0));
    track.setPixelColor(9, track.Color(255, 255, 0));

    track.show();
    tone(PIN_AUDIO, 600);
    delay(2000);
    noTone(PIN_AUDIO);
    for (int i = 0; i < 12; i++)
    {
        track.setPixelColor(i, track.Color(0, 0, 0));
    };

    track.show();
    track.setPixelColor(9, track.Color(0, 0, 0));
    track.setPixelColor(10, track.Color(0, 0, 0));
    track.setPixelColor(8, track.Color(0, 255, 0));
    track.setPixelColor(7, track.Color(0, 255, 0));
    track.show();
    tone(PIN_AUDIO, 1200);
    delay(2000);
    noTone(PIN_AUDIO);

    timestamp = 0;
};

void finish_game(int player_who_won)
{
    uint32_t winner_color = track.Color(0, 0, 0);

    if (player_who_won == 1)
        winner_color = COLOR_PLAYER1;
    if (player_who_won == 2)
        winner_color = COLOR_PLAYER2;

    for (int i = 0; i < TOTAL_LEDS_STRIP; i++)
        track.setPixelColor(i, winner_color);

    track.show();
    winner_fx();
    reset_race_data();
    start_race();
}

void update_players_position()
{
    dist1 += speed_player1;
    dist2 += speed_player2;
}

void calculate_player1_position_delta()
{
    if ((flag_sw1 == 1) && (digitalRead(PIN_PLAYER1) == 0))
    {
        flag_sw1 = 0;
        speed_player1 += ACCELERATION;
    };

    if ((flag_sw1 == 0) && (digitalRead(PIN_PLAYER1) == 1))
    {
        flag_sw1 = 1;
    };

    if ((gravity_map[(word)dist1 % TOTAL_LEDS_STRIP]) < DEF_GRAVITY)
        speed_player1 -= GRAVITY * (DEF_GRAVITY - (gravity_map[(word)dist1 % TOTAL_LEDS_STRIP]));

    if ((gravity_map[(word)dist1 % TOTAL_LEDS_STRIP]) > DEF_GRAVITY)
        speed_player1 += GRAVITY * ((gravity_map[(word)dist1 % TOTAL_LEDS_STRIP]) - DEF_GRAVITY);

    speed_player1 -= speed_player1 * FRICTION;
}

void calculate_player2_position_delta()
{
    if ((flag_sw2 == 1) && (digitalRead(PIN_PLAYER2) == 0))
    {
        flag_sw2 = 0;
        speed_player2 += ACCELERATION;
    };

    if ((flag_sw2 == 0) && (digitalRead(PIN_PLAYER2) == 1))
    {
        flag_sw2 = 1;
    };

    if ((gravity_map[(word)dist2 % TOTAL_LEDS_STRIP]) < DEF_GRAVITY)
        speed_player2 -= GRAVITY * (DEF_GRAVITY - (gravity_map[(word)dist2 % TOTAL_LEDS_STRIP]));

    if ((gravity_map[(word)dist2 % TOTAL_LEDS_STRIP]) > DEF_GRAVITY)
        speed_player2 += GRAVITY * ((gravity_map[(word)dist2 % TOTAL_LEDS_STRIP]) - DEF_GRAVITY);

    speed_player2 -= speed_player2 * FRICTION;
}

void set_ramp(byte H, byte ramp_start, byte ramp_center, byte ramp_end) // gets the position of leds for the start, center and end of the ramp -- H seems to be a hardcoded constant to simulate acceleration physics
{
    for (int i = 0; i < (ramp_center - ramp_start); i++)
    {
        gravity_map[ramp_start + i] = DEF_GRAVITY - i * ((float)H / (ramp_center - ramp_start));
    };
    gravity_map[ramp_center] = DEF_GRAVITY;
    for (int i = 0; i < (ramp_end - ramp_center); i++)
    {
        gravity_map[ramp_center + i + 1] = DEF_GRAVITY + H - i * ((float)H / (ramp_end - ramp_center));
    };
}

void set_loop(byte H, byte a, byte b, byte c)
{
    for (int i = 0; i < (b - a); i++)
    {
        gravity_map[a + i] = DEF_GRAVITY - i * ((float)H / (b - a));
    };

    gravity_map[b] = 255;

    for (int i = 0; i < (c - b); i++)
    {
        gravity_map[b + i + 1] = DEF_GRAVITY + H - i * ((float)H / (c - b));
    };
}

void draw_car1(void)
{
    for (int i = 0; i <= laps_player1; i++)
    {
        track.setPixelColor(((word)dist1 % TOTAL_LEDS_STRIP) + i, track.Color(0, 255 - i * 20, 0));
    };
}

void draw_car2(void)
{
    for (int i = 0; i <= laps_player2; i++)
    {
        track.setPixelColor(((word)dist2 % TOTAL_LEDS_STRIP) + i, track.Color(255 - i * 20, 0, 0));
    };
}

void set_track_base_color()
{
    for (int i = 0; i < TOTAL_LEDS_STRIP; i++)
    {
        track.setPixelColor(i, track.Color(0, 0, (DEF_GRAVITY - gravity_map[i]) / 8));
    };
}

void update_leader()
{
    if (dist1 > dist2)
        leader = 1;
    else
        leader = 2;
}

void update_tbeep()
{
    if (TBEEP > 0)
    {
        TBEEP -= 1;
        if (TBEEP == 0)
        {
            noTone(PIN_AUDIO);
        }; // lib conflict !!!! interruption off by neopixel
    }
}

void check_players_new_lap()
{
    if (dist1 > TOTAL_LEDS_STRIP * laps_player1)
    {
        laps_player1++;
        tone(PIN_AUDIO, 600);
        TBEEP = 2;
    }

    if (dist2 > TOTAL_LEDS_STRIP * laps_player2)
    {
        laps_player2++;
        tone(PIN_AUDIO, 700);
        TBEEP = 2;
    }
}

void check_for_finished_race()
{
    if (laps_player1 > laps_max)
    {
        finish_game(1);
    }

    if (laps_player2 > laps_max)
    {
        finish_game(2);
    }
}

void change_draworder()
{
    if ((millis() & 512) == (512 * draworder))
    {
        if (draworder == 0)
        {
            draworder = 1;
        }
        else
        {
            draworder = 0;
        }
    }
}

void draw_cars()
{
    change_draworder();

    if (draworder == 0)
    {
        draw_car1();
        draw_car2();
    }
    else
    {
        draw_car2();
        draw_car1();
    }

    track.show();
}

void burning1()
{
    //to do
}

void burning2()
{
    //to do
}

void track_rain_fx()
{
    //to do
}

void track_oil_fx()
{
    //to do
}

void track_snow_fx()
{
    //to do
}

void fuel_empty()
{
    //to do
}

void fill_fuel_fx()
{
    //to do
}

void in_track_boxs_fx()
{
    //to do
}

void pause_track_boxs_fx()
{
    //to do
}

void flag_boxs_stop()
{
    //to do
}

void flag_boxs_ready()
{
    //to do
}

void draw_safety_car()
{
    //to do
}

void telemetry_rx()
{
    //to do
}

void telemetry_tx()
{
    //to do
}

void telemetry_lap_time_car1()
{
    //to do
}

void telemetry_lap_time_car2()
{
    //to do
}

void telemetry_record_lap()
{
    //to do
}

void telemetry_total_time()
{
    //to do
}
/* TBD... commented because lack of return caused warnings
int read_sensor(byte player)
{
    //to do
}

int calibration_sensor(byte player)
{
    //to do
}

int display_lcd_laps()
{
    //to do
}

int display_lcd_time()
{
    //to do
}
*/

void setup()
{

    Serial.begin(115200);

    for (int i = 0; i < TOTAL_LEDS_STRIP; i++)
    {
        gravity_map[i] = DEF_GRAVITY;
    };

    track.begin();

    pinMode(PIN_PLAYER1, INPUT_PULLUP);
    pinMode(PIN_PLAYER2, INPUT_PULLUP);

    if ((digitalRead(PIN_PLAYER1) == 0)) //push switch 1 on reset for activate physic
    {
        set_ramp(12, 90, 100, 110); // ramp centred in LED 100 with 10 led fordward and 10 backguard

        for (int i = 0; i < TOTAL_LEDS_STRIP; i++)
        {
            track.setPixelColor(i, track.Color(0, 0, (DEF_GRAVITY - gravity_map[i]) / 8));
        };

        track.show();
    };

    start_race();

    Serial.println("setup ended");
}

void loop()
{
    Serial.println("-----");
    Serial.println("loopstarts");
    //Serial.println(digitalRead(PIN_PLAYER1));

    set_track_base_color();

    calculate_player1_position_delta();

    calculate_player2_position_delta();

    update_players_position();

    update_leader();

    check_players_new_lap();

    check_for_finished_race();

    draw_cars();

    delay(DELAY_MS);

    update_tbeep();
}
