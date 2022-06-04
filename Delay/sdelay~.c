#include "m_pd.h"
#include <math.h>
#include <stdlib.h>
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define SAMPLERATE 44100

/* ------------------------ sdelay~ ----------------------------- */

static t_class *sdelay_class;
typedef struct _sdelay
{
    t_object x_obj;
    t_float x_f;
    t_float FeedBCrossFadeVal;
    t_float sample_rate;
    t_float tapeDelayGoal;
    
    //buffers
    t_float *delayBuffer;
    
    long DELAYSIZE, DELAYMASK;

    //pointers
    int writePointer;
    
    t_float delayTime;
    
} t_sdelay;

//This is the second inlet where you set a feedback delay value.
static void sdelay_float(t_sdelay *x, t_floatarg f)
{
    //this inlet controls the feedback value.
    if(f < 0 || f > 1)
    {
        post("Please keep the values between 0 and 1.");
        return;
    }else
        x->FeedBCrossFadeVal = f;
}

//third inles allowing you to control
static void sdelay_in3(t_sdelay *x, t_floatarg g)
{
    //this inlet controls the delay time.
    if(g < 0 || g > 3)
    {
        post("Please keep the values between 0 and 3.");
        return;
    }else
        x->tapeDelayGoal = g;
}

static t_int *sdelay_perform(t_int *w)
{
    t_sdelay *x = (t_sdelay *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    
    int n = (int)(w[4]);
    
    float crossFade = x->FeedBCrossFadeVal;
    
    //this is the statevariable for the tape delay style effect
    float tapeDelayGoal = x->tapeDelayGoal;
    int readPointer = 0;
    
    while (n--)
    {
        float dry = *(in++);
        int writehead = x->writePointer;
        float delayTime = x->delayTime;
        long delaytrail = (long)(delayTime * x->sample_rate);
        
        //tape delay style effect
        if(delayTime < tapeDelayGoal)
            x->delayTime = x->delayTime + 0.00001;
        
        if(delayTime > tapeDelayGoal)
            x->delayTime = x->delayTime - 0.00001;
        
        //wrap the circlular delay head
        if(writehead < delaytrail)
            readPointer = (2098151 - delaytrail) + writehead;
        else
            readPointer = writehead - delaytrail;

        //read the delayed input from bugger
        float delayout = *(x->delayBuffer + readPointer);

        //write to the delay
        *(x->delayBuffer + writehead) = dry + (delayout * crossFade);
        x->writePointer++;
        
        //check to make sure the delay is wrapping
        if(x->writePointer > 2098151)
            x->writePointer = 0;

        *out++ = dry + (delayout * crossFade);
    }
    
    return (w+5);
}

    
static void sdelay_dsp(t_sdelay *x, t_signal **sp)
{
    //this grabs the sample rate
    x->sample_rate = sp[0]->s_sr;
    dsp_add(sdelay_perform, 4,x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void *sdelay_new(void)
{
    t_sdelay *x = (t_sdelay *)pd_new(sdelay_class);
        
        //add two inlets
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("float"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in3"));
        
        //one outlet
        outlet_new(&x->x_obj, gensym("signal"));
        
        //intialize the struct values
        x->x_f = 0;
        x->writePointer = 0;
        
        // set reasonable starting values
        x->delayTime = 0.5f;
        x->tapeDelayGoal = 0.5f;
        x->DELAYSIZE = 2098152;
        x->DELAYMASK = 2098151;
            
        //allocate the delay buffer
        x->delayBuffer = (t_float *)malloc(x->DELAYSIZE * sizeof(t_float));
        
        //clean the buffer of junk
        for(int i=0; i < 2098152; i++)
                *(x->delayBuffer+i) = 0.0f;
    
    return (x);
}

static void sdelay_free(t_sdelay *x)
{
    free(x->delayBuffer);
}

void sdelay_tilde_setup(void)
{
    sdelay_class = class_new(gensym("sdelay~"), (t_newmethod)sdelay_new, (t_method)sdelay_free, sizeof(t_sdelay), 0, A_DEFFLOAT, 0);
    //left most inlet is the main inlet.
    CLASS_MAINSIGNALIN(sdelay_class, t_sdelay, x_f);
    
    //dsp method
    class_addmethod(sdelay_class, (t_method)sdelay_dsp, gensym("dsp"), 0);
    
    //two control methods
    class_addmethod(sdelay_class, (t_method)sdelay_float, gensym("float"), A_FLOAT, 0);
    class_addmethod(sdelay_class, (t_method)sdelay_in3, gensym("in3"), A_FLOAT, 0);
}
