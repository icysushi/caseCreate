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

int generate(int type, int range_l, int die_height){// type arg is for more complex case generation in the future
    uniform_int_distribution<> m_w(range_l, die_height);
    return m_w(rng);
}

int minimun_space;
int num_macro;
int num_shape;
int die_width;
int die_height;
int dbu_per_micron;
int powerplan;
int buffer;
int alpha;
int beta;
int num_fix;

string def_filename;
string lef_filename;
string txt_filename;
string casename;

class shape{
public:
    double *w;
    double *h;
    int num_big_macro;
    //shape(){}

    shape(){//constructor
        w = new double[num_shape];
        h = new double[num_shape];
        // int temp = generate(0, 1, num_shape/3);
        // for(int i=0;i<num_shape;i++){
        //     if(i<temp){
        //         generateBig(i);
        //     }else{
        //         generateSmall(i);
        //     }
        // }
    }

    void reshape(){//constructor
        delete [] w;
        delete [] h;
        w = new double[num_shape];
        h = new double[num_shape];
        num_big_macro = generate(0, 1, num_shape/2);
        for(int i=0;i<num_shape;i++){
            if(i<num_big_macro){
                generateBig(i);
            }else{
                generateSmall(i);
            }
        }
    }

    void generateBig(int i){
        w[i] = generate(0, int(0.3*die_width), int(0.8*die_width)) /2;
        h[i] = generate(0, int(0.3*die_height), int(0.8*die_height)) /2;
    }

    void generateSmall(int i){
        w[i] = generate(0, int(0.1*die_width), int(0.2*die_width)) /2;
        h[i] = generate(0, int(0.1*die_height), int(0.2*die_height)) /2;
    }
};



bool overlape(int a_x_l, int a_x_h, int a_y_l, int a_y_h, int b_x_l, int b_x_h, int b_y_l, int b_y_h){
    return !( ( (a_x_h<=b_x_l-minimun_space)||(a_x_l-minimun_space>=b_x_h) ) || 
    ((a_y_h<=b_y_l-minimun_space)||(a_y_l-minimun_space>=b_y_h) ));
}


/// different generate mode
void origin_generate(shape& shapes);
void diearea_generate(shape& shapes);

int main(int argc, char *argv[]){
    //set case arguments
    num_macro = atoi(argv[2]);
    num_shape = atoi(argv[3]);
    die_width = 2000;
    die_height = 2000;
    dbu_per_micron = 1000;
    powerplan = 30;
    minimun_space = 10;
    buffer = 70;
    alpha = 1;
    beta = 8;

    num_fix = generate(0, num_macro*0.1, num_macro*0.2);//random

    casename = string(argv[1]);

    def_filename = "case"+casename+".def";
    lef_filename = "case"+casename+".lef";
    txt_filename = "case"+casename+".txt";
    
    shape shapes;

    origin_generate(shapes);

    ofstream f;


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


void diearea_generate(shape& shapes){
    num_fix = generate(0, num_macro*0.1, num_macro*0.2);//random
    poly fix_macro[num_fix];

    int this_shape;
    bool _overlape;
    bool impossible = true;
    long long int area;

    ofstream f;
    while(impossible){
        impossible = false;
        area = 0;

        f.open(def_filename.c_str());
        if (!f.good())
        {
            cerr << "Unable to open def";
        }

        shapes.reshape();

        f << "VERSION 5.7 ;"<<"\n";
        f << "DESIGN case"<< casename << " ;"<<"\n";
        f << "UNITS DISTANCE MICRONS 1000 ;"<<"\n\n";
        f << "DIEAREA ( 0 0 ) ( "<<die_width*dbu_per_micron <<" "<<die_height*dbu_per_micron<<" ) ;"<<"\n";

        f << "\nCOMPONENTS " << num_macro << " ;\n";

        for (int i = 0; i < num_macro; i++)
        {
            this_shape = generate(0, 0,num_shape-1);
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
                            cout<<"fix macro overlape:"<<i<<" "<<j<<", re-assigning..."<<endl;
                            break;
                        }
                    }
                }
                f << fix_macro[i].x_l << " " << fix_macro[i].y_l << " ) N ;\n";
                area += 2*shapes.w[this_shape]*shapes.h[this_shape]//fix macro size *2 because fix macro cause more unplaceble area
                 + 2*(shapes.w[this_shape]*minimun_space + shapes.h[this_shape]*minimun_space);
            }
            else
            {
                f << "PLACED ( ";
                f << generate(0, 0-shapes.w[this_shape]*dbu_per_micron, die_width*dbu_per_micron) << " " 
                << generate(0, 0-shapes.h[this_shape]*dbu_per_micron, die_height*dbu_per_micron) << " ) N ;\n";

                area += shapes.w[this_shape]*shapes.h[this_shape]
                 + 2*(shapes.w[this_shape]*minimun_space + shapes.h[this_shape]*minimun_space);
            }
            
        }

        f << "END COMPONENTS\n\n\nEND DESIGN\n\n\n";
        f.close();

        if(area>(die_height*die_width)){
            cout<<"case impossiple to solve"<<endl;
            cout<<"    macros_area:"<<area<<" die:"<<die_width*die_height<<endl;
            cout<<"    regenerate all..."<<endl;
            impossible = true;
        }
    }
}


void origin_generate(shape& shapes){
    num_fix = generate(0, num_macro*0.1, num_macro*0.2);//random
    poly fix_macro[num_fix];

    int this_shape;
    bool _overlape;
    bool impossible = true;
    long long int area;

    ofstream f;
    while(impossible){
        impossible = false;
        area = 0;

        f.open(def_filename.c_str());
        if (!f.good())
        {
            cerr << "Unable to open def";
        }

        shapes.reshape();

        f << "VERSION 5.7 ;"<<"\n";
        f << "DESIGN case"<< casename << " ;"<<"\n";
        f << "UNITS DISTANCE MICRONS 1000 ;"<<"\n\n";
        f << "DIEAREA ( 0 0 ) ( "<<die_width*dbu_per_micron <<" "<<die_height*dbu_per_micron<<" ) ;"<<"\n";

        f << "\nCOMPONENTS " << num_macro << " ;\n";

        for (int i = 0; i < num_macro; i++)
        {
            this_shape = generate(0, 0,num_shape-1);
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
                            cout<<"fix macro overlape:"<<i<<" "<<j<<", re-assigning..."<<endl;
                            break;
                        }
                    }
                }
                f << fix_macro[i].x_l << " " << fix_macro[i].y_l << " ) N ;\n";
                area += 2*shapes.w[this_shape]*shapes.h[this_shape]//fix macro size *2 because fix macro cause more unplaceble area
                 + 2*(shapes.w[this_shape]*minimun_space + shapes.h[this_shape]*minimun_space);
            }
            else
            {
                f << "PLACED ( ";
                f << generate(0, 0-shapes.w[this_shape]*dbu_per_micron, die_width*dbu_per_micron) << " " 
                << generate(0, 0-shapes.h[this_shape]*dbu_per_micron, die_height*dbu_per_micron) << " ) N ;\n";

                area += shapes.w[this_shape]*shapes.h[this_shape]
                 + 2*(shapes.w[this_shape]*minimun_space + shapes.h[this_shape]*minimun_space);
            }
            
        }

        f << "END COMPONENTS\n\n\nEND DESIGN\n\n\n";
        f.close();

        if(area>(die_height*die_width)){
            cout<<"case impossiple to solve"<<endl;
            cout<<"    macros_area:"<<area<<" die:"<<die_width*die_height<<endl;
            cout<<"    regenerate all..."<<endl;
            impossible = true;
        }
    }
}