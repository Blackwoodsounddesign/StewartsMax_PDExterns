#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define tablesize (1024)
#define twoPI (6.28318530718)

/* ------------------------ swavetable~ ----------------------------- */

static t_class *swavetable_class;

typedef struct _swavetable
{
    // object
    t_object x_obj;
    
    // one over samplerate
    double OneO_N;
    t_float x_f;
    
    t_float Freq;
    t_float phasor;
    double *sinetable;
    
} t_swavetable;

void swavetable_float(t_swavetable *x, t_floatarg f)
{
    // input function for frequency
    
    if(f < 0 || f > 20000)
    {
        post("Please keep the values between 0 and 20000.");
        return;
    }
    else
        x->Freq = f;

}

static t_int *swavetable_perform(t_int *w)
{
    t_swavetable *x = (t_swavetable *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    
    int n = (int)(w[4]);
    double OneO_N = x->OneO_N;
    float freq = x->Freq;
    
    while (n--)
    {
        // the output variable is allocated here to keep it in scope
        float waveout;
        
        // creating the phaser
        if(x->phasor + (OneO_N * freq)  <= 1.0)
            x->phasor += (OneO_N * freq);
        else
            x->phasor += -1.0 + (OneO_N * freq);
        
        // variables for linear interpolation
        float index = (x->phasor * 1024.0);
        int indextrunc = index;
        double delta = index - indextrunc;
        
        // linear interpolation
        if(indextrunc == tablesize - 1)
            waveout = ((1 - delta) * *(x->sinetable + indextrunc)) + (delta * *(x->sinetable));
        else
            waveout = ((1 - delta) * *(x->sinetable + indextrunc)) + (delta * *(x->sinetable + (indextrunc + 1)));
        
        // random unallocated input
        float f = *(in++);
        
        // output
        *out++ = waveout;
        
    }
    return (w+5);
}


static void swavetable_dsp(t_swavetable *x, t_signal **sp)
{
    // the magic behind pd externs
    dsp_add(swavetable_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
    
    // this grabs the sample rate
    x->OneO_N = 1 / sp[0]->s_sr;
}

static void *swavetable_new(void)
{
    t_swavetable *x = (t_swavetable *)pd_new(swavetable_class);
    
    
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("float"));
    outlet_new(&x->x_obj, gensym("signal"));
    
    x->Freq = 0;
    x->x_f = 0;
    
    // allocate a table in memory
    x->sinetable = (double *)malloc(tablesize * sizeof(double));
    
    // fill the wavetable with a sine function from zero to 2 pi
    for(int i = 0; i < tablesize; i++)
        *(x->sinetable+i) = sin((twoPI*i)/tablesize);
    
    return (x);
}

static void swavetable_free(t_swavetable *x)
{
    // deallocate the memory
    free(x->sinetable);
}

void swavetable_tilde_setup(void)
{
    // this function creates a C class
    swavetable_class = class_new(gensym("swavetable~"), (t_newmethod)swavetable_new, (t_method)swavetable_free,
        sizeof(t_swavetable), 0, A_DEFFLOAT, 0);

    CLASS_MAINSIGNALIN(swavetable_class, t_swavetable, x_f);
    
    // these adds methods to our object
    class_addmethod(swavetable_class, (t_method)swavetable_dsp, gensym("dsp"), 0);
    class_addmethod(swavetable_class, (t_method)swavetable_float, gensym("float"), A_FLOAT, 0);
}
