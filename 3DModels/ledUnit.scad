$fn=50;
D=3.45;
H=15;
R=25.5;

unit();

module unit() rotate([0,180,45]) {
    for(i=[0:3]) rotate([0,0,i*90]) translate([R/2,0])
        union() hull() {  
            cylinder(h=H+2,d=D);
            rotate([45,0]) cube([0.1,2,2],center=true);
          
        }
        
    translate([0,0,H])
    rotate([0,0,45]) linear_extrude(2)
    difference() {
        offset(D/2) square(18,center=true);
        square(12,center=true);
    }
}