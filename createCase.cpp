#include <fstream>
#include <iostream>
#include <cstdlib>
#include <random>
#include <string.h>
#include <ctime>




using namespace std;

//random_device rd; 
mt19937_64 rng(time(NULL));

struct poly{
    int x_l;
    int x_h;
    int y_l;
    int y_h;
};

class shape{
public:
    double *w;
    double *h;
    shape(){}

    shape(int num, int range_w, int range_h){
        uniform_int_distribution<> s_w(20, int(0.1*range_w));
        uniform_int_distribution<> s_h(20, int(0.1*range_h));//look comment below
        w = new double[num];
        h = new double[num];
        for(int i=0;i<num;i++){
            w[i] = s_w(rng) / 2;
            h[i] = s_h(rng) / 2;
        }//max are 0.05 die_size(*0.1 and /2), because I think they shouldn't be too large
        //  and devide 2 make some of them have 小數
    }
};

int generate(int type, int range_l, int range_h){// type arg is for more complex case generation in the future
    uniform_int_distribution<> m_w(range_l, range_h);
    return m_w(rng);
}

bool overlape(int a_x_l, int a_x_h, int a_y_l, int a_y_h, int b_x_l, int b_x_h, int b_y_l, int b_y_h){
    return !( ( (a_x_h<=b_x_l)||(a_x_l>=b_x_h) ) || ((a_y_h<=b_y_l)||(a_x_l>=b_x_h) ));
}

int main(int argc, char *argv[]){
    //set case arguments
    int num_macro = atoi(argv[2]);
    int num_shape = atoi(argv[3]);
    int die_width = 50000;
    int die_height = 40000;
    int dbu_per_micron = 1000;
    int powerplan = 30;
    int minimun_space = 10;
    int buffer = 70;
    int alpha = 1;
    int beta = 8;

    int num_fix = generate(0, num_macro*0.1, num_macro*0.2);

    string def_filename = "case"+string(argv[1])+".def";
    string lef_filename = "case"+string(argv[1])+".lef";
    string txt_filename = "case"+string(argv[1])+".txt";
    
    shape shapes(num_shape, die_width, die_height);

    poly fix_macro[num_fix];


    ofstream f(def_filename.c_str());
    if (!f.good())
    {
        cerr << "Unable to open def";
    }

    f << "VERSION 5.7 ;"<<"\n";
    f << "DESIGN case"<< argv[1] << " ;"<<"\n";
    f << "UNITS DISTANCE MICRONS 1000 ;"<<"\n\n";
    f << "DIEAREA ( 0 0 ) ( "<<die_width*dbu_per_micron <<" "<<die_height*dbu_per_micron<<" ) ;"<<"\n";

    f << "\nCOMPONENTS " << num_macro << " ;\n";

    int this_shape;
    uniform_int_distribution<> m_shape(0,num_shape-1);
    bool _overlape;
    for (int i = 0; i < num_macro; i++)
    {
        this_shape = m_shape(rng);
        f << "   - " << "M"<<i<<"_C"<<this_shape //name:Mx_Cy
          << " " << "C"<<this_shape << " \n"  //shape name:Cy
          << "      + ";
        
        //uniform_int_distribution<> rand_int(0, 1000);
        if (i<num_fix)
        {
            f << "FIXED ( ";
            _overlape = true;
            while(_overlape){
                _overlape = false;
                fix_macro[i].x_l = generate(0, 0, (die_width-shapes.w[this_shape])*dbu_per_micron);
                fix_macro[i].y_l = generate(0, 0, (die_height-shapes.h[this_shape])*dbu_per_micron);
                fix_macro[i].x_h = fix_macro[i].x_l + shapes.w[this_shape]*dbu_per_micron;
                fix_macro[i].y_h = fix_macro[i].y_l + shapes.h[this_shape]*dbu_per_micron;
                for(int j=0;j<i;j++){
                    if(overlape(fix_macro[i].x_l, fix_macro[i].x_h, fix_macro[i].y_l, fix_macro[i].y_h, fix_macro[j].x_l, fix_macro[j].x_h, fix_macro[j].y_l, fix_macro[j].y_h)){
                        _overlape = true;
                        cout<<i<<" "<<j<<endl;
                        break;
                    }
                }
            }
            f << fix_macro[i].x_l << " " << fix_macro[i].y_l << " ) N ;\n";
        }
        else
        {
            f << "PLACED ( ";
            f << generate(0, 0-shapes.w[this_shape]*dbu_per_micron, die_width*dbu_per_micron) << " " << generate(0, 0-shapes.h[this_shape]*dbu_per_micron, die_height*dbu_per_micron) << " ) N ;\n";
        }
        
    }

    f << "END COMPONENTS\n\n\nEND DESIGN\n\n\n";
    f.close();



//create lef
    f.open(lef_filename.c_str());
    if (!f.good())
    {
        cerr << "Unable to open lef";
    }
    for(int i=0;i<num_shape;i++){
        f<<"MACRO C"<<i<<"\n";
        f<<"     SIZE "<<shapes.w[i]<<" BY "<<shapes.h[i]<<" ;"<<"\n";
        f<<"END C"<<i<<"\n\n";
    }
    f<<"END LIBRARY"<<"\n";
    f.close();

//create txt
    f.open(txt_filename.c_str());
    if (!f.good())
    {
        cerr << "Unable to open txt";
    }
    f << "powerplan_width_constraint "<<powerplan<<"\n"
    <<"minimum_channel_spacing_between_macros_constraint "<<minimun_space<<"\n"
    <<"buffer_area_reservation_extended_distance_constraint "<<buffer<<"\n"
    <<"weight_alpha "<<alpha<<"\n"
    <<"weight_beta "<<beta<<"\n";
    f.close();

    return 0;
}