#include "m_pd.h"
#include <math.h>
#include <stdlib.h>
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define tablesize (1024)
#define twoPI (6.28318530718)

/* ------------------------ TapeDelay~ ----------------------------- */

static t_class *TapeDelay_class;
typedef struct _TapeDelay
{
    //various places to store the data
    t_object x_obj;
    t_float x_f;
    t_float FeedBCrossFadeVal;
    t_float sample_rate;
    t_float delstateVar;
    t_float filterCoeff;
    
    //freq for the LFO
    t_float freq;
    
    //LFO wavetable variables
    double *sinetable;
    t_float phasor;
    double OneO_N;
    
    //tape delay head smoothing variable
    double SmoothVar;
    
    //buffers
    t_float *delayBuffer;
    t_float filterSample;
    
    long DELAYSIZE, DELAYMASK;

    //pointer
    int writePointer;
    t_float delayTime;
    
} t_TapeDelay;

static void TapeDelay_float(t_TapeDelay *x, t_floatarg f)
{
    //this inlet controls the feedback value & delay volume.
    if(f < 0 || f > 1)
    {
        post("Please keep the values between 0 and 1.");
        return;
    }else
        x->FeedBCrossFadeVal = f;

}

static void TapeDelay_in3(t_TapeDelay *x, t_floatarg g)
{
    //this inlet controls the delay time.
    if(g < 0.25 || g > 3)
    {
        post("Please keep the values between 0.25 and 3.");
        return;
    }else
        x->delstateVar = g;

}

static void TapeDelay_in4(t_TapeDelay *x, t_floatarg t)
{
    //this inlet controls the lowpass filter coefficent.
    if(t < 0 || t > 20000)
    {
        post("Please keep the values between 0 and 20000.");
        return;
    }else
        x->filterCoeff = exp(-2.0 * M_PI * (t/x->sample_rate));

}

static void TapeDelay_in5(t_TapeDelay *x, t_float q)
{
    //this inlet controls the tape delay interpolation time.
    if(q < 1 || q > 9)
    {
        post("Please keep the values between 1 and 9.");
        return;
    }else
        x->SmoothVar = 0.000001 * (double)q;

}

static void TapeDelay_in6(t_TapeDelay *x, t_float j)
{
    //this inlet controls the distortion variable.
    if(j < 0 || j > 10)
    {
        post("Please keep the values between 1 and 10.");
        return;
    }else
        x->freq = j;

}

static t_int *TapeDelay_perform(t_int *w)
{
    t_TapeDelay *x = (t_TapeDelay *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    
    int n = (int)(w[4]);
    
    //making a blank read pointer
    int readPointer = 0;
    
    //crossfade delay variable
    float crossFade = x->FeedBCrossFadeVal;
    
    //this is the end position of the tape delay head
    float delstateVar = x->delstateVar;
    double SmoothVar = x->SmoothVar;
    
    //filter variable
    float filterCoeff = x->filterCoeff;
    float a0 = 1.0 - filterCoeff;
    
    //LFO variable
    float freq = x->freq;
    
    //LFO
    double OneO_N = x->OneO_N;
    
    while (n--)
    {
        //dry input signal
        float dry = *(in++);
        
// ======== DelaySection ======== //
        
            //writehead pointer
            int writehead = x->writePointer;
            
            //setting a delay time
            float delayTime = x->delayTime;
            long delaytrail = (long)(delayTime * x->sample_rate);
            
            //tape delay style effect with control
            if(delayTime < delstateVar)
                x->delayTime = x->delayTime + (0.00001 - SmoothVar);
            
            if(delayTime > delstateVar)
                x->delayTime = x->delayTime - (0.00001 - SmoothVar);
            
            //wrap the circlular delay head
            if(writehead < delaytrail)
                readPointer = (2098151 - delaytrail) + writehead;
            else
                readPointer = writehead - delaytrail;

            //read the delayed input from bugger
            float delayout = *(x->delayBuffer + readPointer);
    
// ===== Low Pass= === // filter applied to the delayed sound
        
        delayout = delayout * a0 + x->filterSample * filterCoeff;
        x->filterSample = delayout;
            
            //write to the delay
            *(x->delayBuffer + writehead) = dry + (delayout * crossFade);
            x->writePointer++;
            
            //check to make sure the delay is wrapping
            if(x->writePointer > 2098151)
                x->writePointer = 0;

// ====== LFO Wavetable ====== //
        
        //phasor
        if(x->phasor + OneO_N > 1)
            x->phasor = 0;
        else
            x->phasor = x->phasor + (OneO_N * freq);
        
        int index = (int)(x->phasor * tablesize);
        float LFO = *(x->sinetable + index);
        
// ===== Distortion & Output ===== //
        
        //distortion just on the output & add a bit of AM modulation using a wavetable
        if(freq > 0.0001)
            *out++ = (atan((dry + (delayout * crossFade))) * 2/M_PI) * LFO;
        else
            *out++ = (atan((dry + (delayout * crossFade))) * 2/M_PI);  
    }
    
    return (w+5);
}


static void TapeDelay_dsp(t_TapeDelay *x, t_signal **sp)
{
    //get the sample rate
    x->sample_rate = sp[0]->s_sr;
    x->OneO_N = 1 / sp[0]->s_sr;
    dsp_add(TapeDelay_perform, 4,x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void *TapeDelay_new(void)
{
    t_TapeDelay *x = (t_TapeDelay *)pd_new(TapeDelay_class);
        
        //inlets
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("float"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in3"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in4"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in5"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in6"));
        
        //outlets
        outlet_new(&x->x_obj, gensym("signal"));
        
        //intialize the struct values
        x->x_f = 0;
        x->writePointer = 0;
        x->filterCoeff = 0.0f;
        x->freq = 0.0f;
    
        x->delayTime = 0.5f;
        x->delstateVar = 0.5f;
        
        x->DELAYSIZE = 2098152;
        x->DELAYMASK = 2098151;
        x->SmoothVar = 0.000001;
            
        //allocate the delay buffer
        x->delayBuffer = (t_float *)malloc(x->DELAYSIZE * sizeof(t_float));
    
        //fill the buffer with zeros
        for(int i=0; i < 2098152; i++)
            *(x->delayBuffer+i) = 0.0f;
        
        //wavetable memory management
        x->sinetable = (double *)malloc(tablesize * sizeof(double)); // allocate a table
        for(int i = 0; i < tablesize; i++)
            *(x->sinetable+i) = sin((twoPI*i)/1024);
    
    return (x);
}

static void TapeDelay_free(t_TapeDelay *x)
{
    //free the memory we credited
    free(x->delayBuffer);
    free(x->sinetable);
}

void TapeDelay_tilde_setup(void)
{
    TapeDelay_class = class_new(gensym("TapeDelay~"), (t_newmethod)TapeDelay_new, (t_method)TapeDelay_free, sizeof(t_TapeDelay), 0, A_DEFFLOAT, 0);
    
    CLASS_MAINSIGNALIN(TapeDelay_class, t_TapeDelay, x_f);
    
    //generating the inputs
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_dsp, gensym("dsp"), 0);
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_float, gensym("float"), A_FLOAT, 0);
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_in3, gensym("in3"), A_FLOAT, 0);
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_in4, gensym("in4"), A_FLOAT, 0);
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_in5, gensym("in5"), A_FLOAT, 0);
    class_addmethod(TapeDelay_class, (t_method)TapeDelay_in6, gensym("in6"), A_FLOAT, 0);
}
