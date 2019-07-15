/*wrote @ 06/29/2019
wrote by Zheng Zhang
This program is capable for visualisation with ociliscope, if one only wants spike locations computation can be even reduced
i.e. keep slience when holding data*/

/*Notes: mini out ~3.3mV */
/*Sampling freq of source 24414 Hz, Vpp: 500mV~2V, offset 500mV -> offset is important*/

#include "mbed.h"

//Serial pc(USBTX, USBRX);
AnalogIn raw(A0);
//AnalogOut aso_out(DAC0_OUT);
//PwmOut thr(A4);
DigitalOut indicator(A2);
//DigitalOut red(LED1);
//DigitalOut green(LED2);
//DigitalOut blue(LED3);
//Timer timer;
//Timer timer;
//Timeout timer3;
//Timeout timer1;
//Timeout timer2;

Ticker Sampling;
int update = 1;
int hold_data = 0;
int detected = 0;
int begin, end;
void hold(){
    hold_data = 0;
//    green = 1;
//    printf("hold finish!\n");
}

void update_(){
    update = 1;
//    printf("update!\n");
}

void detect(){

//    indicator = 0;
    detected = 0;
    update = 1;
    hold_data = 0;
//    printf("detected!\n");
//    red = 1;
}

// fast division of float and float. Not roubust when exponent is in boundary. Does NOT fast implement with C.
// fastDiv_float(float data, short bits){
//    return ( (short*)&data)[1]-=bits<<7;
//}

//double fastDiv_double(double data, short bits){
//    return ( (short*)&data)[3]-=bits<<4;
//}
    int mean_buffer_size = 16;
    int mean_buffer_end = 0;
    float sum = 0;
    float previous_demean = 0;
    float mean_buffer[16]; // a ring buffer
    
    float aso = 0;
    
    int thr_buffer_size = 64;
    int thr_buffer_end = 0;
    float thr_buffer[64]; // a ring buffer
    float thr_buffer_mean = 0;
    float threshold = 0;
    
    float hold_time = 0.0005;
    float update_period = 0.01;
    float detected_time = 0.0005;
    
    void iter(){
        /*subtract mean*/
        sum=sum-mean_buffer[mean_buffer_end];
        
        indicator = 1;
        mean_buffer[mean_buffer_end] = raw.read(); //20us
        indicator = 0;
        
        indicator = 1;
        sum=sum+mean_buffer[mean_buffer_end]; //substract first item and add the new item. reduce computation by size/2
        float demean;
        if(mean_buffer_end == 0) 
            demean = mean_buffer[mean_buffer_size-1] - sum / 16; 
        else
            demean = mean_buffer[mean_buffer_end-1] -sum / 16;   

        if(mean_buffer_end + 1 == mean_buffer_size) // update mean buffer end
            mean_buffer_end = 0;
        else 
            mean_buffer_end++;
        indicator = 0;
        
//        /*ASO*/
        indicator = 1;    
        aso = 100*demean * ( demean - previous_demean );        
        previous_demean = demean;
        indicator = 0;           
//       // /*Thresholding*/
        indicator = 1;
        if(!detected){
//            indicator = 1;
            if(abs(aso) <= threshold){ //below threshold, not detected
                if(abs(aso) > threshold/2){ // above subthreshold
                    hold_data = 1;
//                    green = 0;
//                    timer1.attach(&hold, hold_time);
                }
                if(update!=20&&!hold_data){ // update threshold                 

                    update=0;
//                    thr_buffer_mean = 0;
//                    for(int i = 0; i < thr_buffer_size; i++)  //9us
//                        thr_buffer_mean+=thr_buffer[i];
//                    thr_buffer_mean = thr_buffer_mean / 64;
                    threshold = 20*thr_buffer_mean;


//                    timer2.attach(&update_, update_period);                
                }
                else 
                    update++; 
            }               
            else{ // above threshold, detected
//            indicator  = 1;
//                red = 0;
//                green = 1;
                detected = 1;
                hold_data = 1;
                update = 0;
//                timer.start();
//                timer3.attach(&detect,detected_time);
            }
        }
        else{ 
//            indicator = 0;
            detected = 0;
            update = 1;
            hold_data = 0;
        }
//        indicator = 1;
        if(hold_data){
            float temp = thr_buffer_mean;
            thr_buffer_mean = (thr_buffer_mean*64 - thr_buffer[thr_buffer_end] + abs(thr_buffer_mean))/64;
            thr_buffer[thr_buffer_end] = abs(temp);
            hold_data = 0; 
        }
        else{
            thr_buffer_mean = (thr_buffer_mean*64 - thr_buffer[thr_buffer_end] + abs(aso))/64;
            thr_buffer[thr_buffer_end] = abs(aso);
        }
//        indicator = 0;
            
//        aso_out.write(aso); 
//        thr.write(threshold);
          
        if(thr_buffer_end+1 == thr_buffer_size) // xupdate thr buffer end
            thr_buffer_end = 0;
        else
            thr_buffer_end++;
//        indicator = 0;
        }
        
int main(){
//    red = 1;
//    thr.period(0.0005); //pwm period
    
//fill buffers
    for(int i = 0; i < mean_buffer_size; i++){
        mean_buffer[i] = raw.read();
        sum += mean_buffer[i];
        wait(0.0004);
    }
    for(int j = 1; j < thr_buffer_size; j++){
        /*subtract mean*/
        sum=sum-mean_buffer[mean_buffer_end];
        mean_buffer[mean_buffer_end] = raw.read();
        sum=sum+mean_buffer[mean_buffer_end]; //substract first item and add the new item. reduce computation by size/2
        float demean;
        if(mean_buffer_end == 0)
            demean = mean_buffer[mean_buffer_size-1] - sum / 16; // /16
        else
            demean = mean_buffer[mean_buffer_end-1] -sum / 16;   // /16
        
        if(mean_buffer_end + 1 == mean_buffer_size)
            mean_buffer_end = 0;
        else 
            mean_buffer_end++;
        /*ASO*/    
        aso = 100*demean * ( demean - previous_demean );
//        aso_out.write(abs(aso));
        thr_buffer[j] = aso;
        previous_demean = demean;
        thr_buffer_mean += thr_buffer[j];
        wait(0.0004);
    }
    thr_buffer_mean = thr_buffer_mean / 64;
    threshold = 3 * abs(thr_buffer_mean); 
//    thr.write(threshold);

    Sampling.attach(&iter, 0.0005); // sample in every 0.4ms
    
    while(1){}
}


