#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define tablesize (1024)

/* ------------------------ dist~ ----------------------------- */

static t_class *swavetable_class;

typedef struct _swavetable
{
    t_object x_obj;
    t_float x_f;
    double OneO_N;
    
    t_float ChebyCrossFadeVal;
    t_float FMCrossFadeVal;
    t_float phasor;
    double *sinetable;
    
    t_float DCval;
    
} t_swavetable;

void swavetable_float(t_swavetable *x, t_floatarg f)
{
    
    if(f < 0 || f > 1)
    {
        post("Please keep the values between 0 and 1.");
        return;
    }else
    {
        x->ChebyCrossFadeVal = f;
    }

}

void swavetable_in3(t_swavetable *x, t_floatarg g)
{
    
    if(g < 0 || g > 300)
    {
        post("Please keep the values between 0 and 1.");
        return;
    }else
    {
        x->FMCrossFadeVal = g;
    }

}

static t_int *swavetable_perform(t_int *w)
{
    t_swavetable *x = (t_swavetable *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    
    int n = (int)(w[4]);
    double OneO_N = x->OneO_N;
    float freq = x->FMCrossFadeVal;
    
    while (n--)
    {
        
        if(x->phasor + (OneO_N * freq)  <= 1.0){
            x->phasor += (OneO_N * freq);
        }else
            x->phasor += -1.0 + (OneO_N * freq);
        
        int index = (x->phasor * 1024.0);
        float waveout;
        int indextrunc = index;
        double delta = index - indextrunc;
        
        if(indextrunc == tablesize - 1)
            waveout = ((1 - delta) * *(x->sinetable + indextrunc)) + (delta * *(x->sinetable));
        else
            waveout = ((1 - delta) * *(x->sinetable + indextrunc)) + (delta * *(x->sinetable + (indextrunc + 1)));
        
        float f = *(in++);
    
         
        *out++ = waveout;
        
    }
    return (w+5);
}


static void swavetable_dsp(t_swavetable *x, t_signal **sp)
{
    dsp_add(swavetable_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
    
    x->OneO_N = 1 / sp[0]->s_sr;
}

static void *swavetable_new(void)
{
    t_swavetable *x = (t_swavetable *)pd_new(swavetable_class);
    
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("float"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in3"));
    
    outlet_new(&x->x_obj, gensym("signal"));
    
    x->x_f = 0;
    x->ChebyCrossFadeVal = 0;
    x->FMCrossFadeVal = 0;
    
    //static int tablesize = 1024;
    static double twoPI = 6.28318530718; // two pi
    
    x->sinetable = (double *)malloc(tablesize * sizeof(double)); // allocate a table
    
    for(int i = 0; i < tablesize; i++)
        *(x->sinetable+i) = sin((twoPI*i)/tablesize);
    
    return (x);
}

static void swavetable_free(t_swavetable *x)
{
    free(x->sinetable);
}

void swavetable_tilde_setup(void)
{
    swavetable_class = class_new(gensym("swavetable~"), (t_newmethod)swavetable_new, (t_method)swavetable_free,
        sizeof(t_swavetable), 0, A_DEFFLOAT, 0);

    CLASS_MAINSIGNALIN(swavetable_class, t_swavetable, x_f);

    class_addmethod(swavetable_class, (t_method)swavetable_dsp, gensym("dsp"), 0);
    class_addmethod(swavetable_class, (t_method)swavetable_float, gensym("float"), A_FLOAT, 0);
    class_addmethod(swavetable_class, (t_method)swavetable_in3, gensym("in3"), A_FLOAT, 0);
}
