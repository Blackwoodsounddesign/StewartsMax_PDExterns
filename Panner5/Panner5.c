#include "m_pd.h"
#include <math.h>

#define TESTVALUE (1.57079632679)

typedef struct Panner5
{
  t_object x_ob;
    
  float f_value;
  float b_value;
  float xcoor;
    
  t_outlet *x_outlet;
  t_outlet *x_outlet2;
  t_outlet *x_outlet3;
  t_outlet *x_outlet4;
  t_outlet *x_outlet5;
    
} t_Panner5;

//main function
void CalculateOutletValues(t_Panner5 *x)
{
    // speakers 1 and 2
    float cosxcoor = cos(x->xcoor) * x->f_value;
    float sinxcoor = sin(x->xcoor) * x->f_value;
    
    //speakers 2 and 3
    float cosxcoor2 = cos(x->xcoor - TESTVALUE) * x->f_value;
    float sinxcoor2 = sin(x->xcoor - TESTVALUE) * x->f_value;
    
    //speakers 4 and 5
    float bcoscoor = cos(x->xcoor * 0.5) * x->b_value;
    float bsincoor = sin(x->xcoor * 0.5) * x->b_value;
    
    
    float normalization = 0;
    
    if(x->xcoor < TESTVALUE)
    {
    outlet_float(x->x_outlet, cosxcoor);
    outlet_float(x->x_outlet2, sinxcoor);
    outlet_float(x->x_outlet3, 0);
    normalization = sqrt( 4 * pow( (cosxcoor * sinxcoor * bsincoor * bcoscoor), 2));
        
    }
    else if (x->xcoor == 1.570796)
    {
        outlet_float(x->x_outlet, 0);
        outlet_float(x->x_outlet2, x->f_value);
        outlet_float(x->x_outlet3, 0);
        normalization = 0;
    }
    else if (x->xcoor > TESTVALUE)
    {
        outlet_float(x->x_outlet, 0);
        outlet_float(x->x_outlet2, cosxcoor2);
        outlet_float(x->x_outlet3, sinxcoor2);
        normalization = sqrt( 4 * pow(((cosxcoor2 * sinxcoor2 * bsincoor * bcoscoor)), 2));
    }
    
    outlet_float(x->x_outlet4, bcoscoor);
    outlet_float(x->x_outlet5, bsincoor);
    
    post("norm: %f", normalization);
}

//left inlet function
void Panner5_float(t_Panner5 *x, t_floatarg f)
{
    x->xcoor = ((((f - 63.5) / 63.5) + 1) * 0.5) * M_PI;
    CalculateOutletValues(x);
}

//right inlet function
void Panner5_rflt(t_Panner5 *x, t_floatarg g)
{
    float controlvar = ((((g - 63.5) / 63.5) + 1) * .5) * TESTVALUE;
    
    x->f_value = sin(controlvar);
    x->b_value = cos(controlvar);
    
    CalculateOutletValues(x);
}

// Pointer to the class //
t_class *Panner5_class;

    //Called when this is instatied//
void *Panner5_new(void)
{
    t_Panner5 *x = (t_Panner5 *)pd_new(Panner5_class);
    
    //Vertical inlet//
    inlet_new(&x->x_ob, &x->x_ob.ob_pd, gensym("float"), gensym("rflt"));
    
    // Outlets//
    x->x_outlet = outlet_new(&x->x_ob, gensym("float"));
    x->x_outlet2 = outlet_new(&x->x_ob, gensym("float"));
    x->x_outlet3 = outlet_new(&x->x_ob, gensym("float"));
    x->x_outlet4 = outlet_new(&x->x_ob, gensym("float"));
    x->x_outlet5 = outlet_new(&x->x_ob, gensym("float"));
    
    post("Please connect two sliders to the panner object");
    
    x->f_value = 0;
    x->b_value = 1;
    x->xcoor = 0;
    
    return (void *)x;
}

void Panner5_setup(void)
{
    post("Panner5_setup");
    
    Panner5_class = class_new(gensym("Panner5"), (t_newmethod)Panner5_new, 0,
    	sizeof(t_Panner5), 0, 0);
    
    class_addmethod(Panner5_class, (t_method)Panner5_rflt, gensym("rflt"), A_FLOAT, 0);
    
    class_addfloat(Panner5_class, Panner5_float);
}

