$fn=50;
D=3.45;
H=15;
G=18;
lw=45;

X=3;
Y=4;

label();
translate([+14,-4,13])rotate([0,-90]) !clip();

module clip() {
    a=4;
    linear_extrude(10)
    difference() {
        square([a*2,5]);
        translate([a/2,2]) square([a,2.0]);
        translate([a/2,2]) square([3,6]);
    }
    
}

module label() {

    union() {
        for(x=[0:X-1],y=[0:Y-1]) translate([x*G,y*G])
        union() hull() {  
            cylinder(h=H+2,d=D);
            rotate([45,0]) cube([0.1,2,2],center=true);
        }
            
        translate([0,0,H])
        linear_extrude(2)
        offset(D/2) square([G*(X-1),G*(Y-1)]);
    }
        
    translate([0,0,H+2.1])
    difference() {
        linear_extrude(2)
        offset(D/2) square([G*(X-1),G*(Y-1)]);
        
        linear_extrude(5,center=true)
        offset(delta=0.5) square([G*(X-1),G*(Y-1)]);
        
        translate([-5/2,G*(Y-1)/2-lw/2,-4.7]) cube([G*(X-1)+lw,lw,5]);
    }

}
