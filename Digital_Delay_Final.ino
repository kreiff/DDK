 /*
 * Inputs: Reads audio from analog input pin A0 and parameter control from A1, A2, & A3
 * Outputs: Writes to DAC output pin DAC0
 * With Microsecond delay writes to serial console once every ~40,000 samples
 * 
 * Processing: An analog signal is read from pin A0.
 * The signal is stored into a ring-buffer after summing it with the previous input
 * that reside at the tap point in the buffer.
 * Tap point values are scaled by < 1 to implement decaying echos
 * or reverborations. After each such operation, a point into the ring buffer is advanced
 * and the process is repeated by returning to the top of the main loop.
 * Sample rate is about 40k / sec and the buffer is 5059 long.
 */

//Setup the ring buffer
#define BSIZE 5095 //Buffer size, sets the maximum delay time.
int ring_buff[BSIZE]; //Ring buffer

const int numReadings = 5000; //number of samples of the pot value for smoothing

int readings[numReadings];      // the readings from the delayTime pot input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;

void setup() {
  
  //Initialize the buffer contents to all zero
  for (int i=0; i<BSIZE; i++){
    ring_buff[i] = 0;
  }
   //Initialize the smoothing buffer contents to all zero
   for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
   }

  //Enable the ADC and DAC pins
  pinMode(A0, INPUT); // Define A0 as ADC input
  pinMode(A1, INPUT); // Define 3 pot inputs
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  analogReadResolution(12); //Override 10 bit default. We can go to 12 bits because the board used is a Due
  analogWriteResolution(12); //Override default 8 bit since we'll use DAC1 output on Due board

  //Open serial monitor transcript
  Serial.begin(115200);
  }

void loop() {
  
  int analogin; //stores ADC data
  float old_value1; // values read from the tap point
  int analogout; //value to be output and stored into buffer at current_tap
  int analogout_buf; //duplicate of analogueout taken before effect mix attenuation
  int current_tap = 0; //The current tap point - incremented after each sample
  int tap; //Temp variable used in indexing into older tap points
  byte effectMix; // variable for setting the effect vs. input mix
  int delayTime; // sets delay time
  float feedbackAmount; // sets feedback amount

  //While loop to improve effect timing
  while (1) {

    total = total - readings[readIndex];
    // read from the delayTime pot:
    readings[readIndex] = analogRead(A1);
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;

    // if we're at the end of the array...
    if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
    average = total / numReadings;
    analogin = analogRead(A0) - 2048; //Making audio values bi-polar

    delayMicroseconds(20);
    
    effectMix = map(analogRead(A3), 0, 4095, 0, 8);
    delayTime = map(average, 0, 4095, 1000,5095); //sets delay time
    feedbackAmount = map(analogRead(A2), 0, 4095, 1,100)*0.1; //reads pot for feedback amount

    tap = current_tap - delayTime; // sets temp tap value and grabs values earlier in the ring buffer
    if (tap < 0){                  //if tap is negative number, pushes it back to the top of the ring buffer
      tap += BSIZE; 
    }
    old_value1 = ring_buff[tap];   //grabs delayed audio for playback
    
    //Scale values by a specfic divisor to determine feedback amount
    old_value1 /= 1.0+feedbackAmount;

    analogout_buf =  analogin + old_value1; //add passthrough audio from ADC to scaled delayed audio

    //Limit the outputs to mimic normal overload distortion
    //avoid digital number wraparound - might not be needed for the buffer?
    if(analogout_buf > 2047){ analogout_buf = 2047;}
    if(analogout_buf < -2047){ analogout_buf = -2047;}
    
    //Effect Level for repeats (i.e. turn down input signal)
    analogin = analogin >> effectMix;
    
    analogout = analogin + old_value1; //add passthrough audio from ADC to scaled delayed audio
    
    //Limit the outputs to mimic normal overload distortion
    //avoid digital number wraparound
    if(analogout > 2047){ analogout = 2047;}
    if(analogout < -2047){ analogout = -2047;}
    
    //store duplicated output in buffer for "effect only" playback
    ring_buff[current_tap] = analogout_buf;
    
    //increment (circular) tap-point
    current_tap++;
    if(current_tap > BSIZE-1){ //if tap point goes beyond BSIZE, start back at 0
      current_tap = 0;
    }
    
    //Scale the value back to an unsigned value in preparation for output to DAC
    analogout += 2048;
    
    //Write to DAC0 output pin on Due board
    analogWrite(DAC0, analogout);

//  ******* For tests only *********
//  *****Comment out to use the delay*****
//    Serial.println(current_tap++);
   }
}
