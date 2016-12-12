board_x=90;
board_y=70;
board_z=2;
board_offset=25;
hole_x_offset=2.1;
hole_y_offset=2.3;
hole=3.1;

mount_z=4;
mount_hole=3;
mount_x=15;
mount_x_offset=(mount_x/2)-hole_x_offset;
mount_y=8;
mount_extension=15;




module pcb() {
    difference() {
        cube(size=[board_x, board_y,board_z]);
        translate([hole_x_offset, hole_y_offset, 0]){
            cylinder(h=board_z , r=(hole/2), ,$fn=30);
        }
        translate([hole_x_offset, board_y-hole_y_offset, 0]){
            cylinder(h=board_z , r=(hole/2), ,$fn=30);
        }
        translate([board_x-hole_x_offset, board_y-hole_y_offset, 0]){
            cylinder(h=board_z , r=(hole/2), ,$fn=30);
        }
        translate([board_x-hole_x_offset, hole_y_offset, 0]){
            cylinder(h=board_z , r=(hole/2), ,$fn=30);
        }
    }
}

module boards() {
    color("SteelBlue", 1) {
        translate([mount_x_offset,mount_y,mount_z]){
            pcb();
        }
        translate([mount_x_offset,mount_y,mount_z+board_offset]){
            pcb();
        }
        translate([mount_x_offset,mount_y,mount_z+(board_offset*2)]){
            pcb();
        }    
        translate([mount_x_offset,mount_y,mount_z+(board_offset*3)]){
            pcb();
        }       
        translate([mount_x_offset,mount_y,mount_z+(board_offset*4)]){
            pcb();
        }    
    }
}



module mount() {
    difference() {
        translate([0,0,0]) {
            cube(size=[mount_x, mount_y, mount_extension+board_z+mount_z+(board_offset*4)]);
            translate([0,0,0]) {
                cube(size=[mount_x, mount_y*2,mount_z]);
            }
            translate([0,0,board_offset]) {
                cube(size=[mount_x, mount_y*2,mount_z]);
            }
            translate([0,0,board_offset*2]) {
                cube(size=[mount_x, mount_y*2,mount_z]);
            }
            translate([0,0,board_offset*3]) {
                cube(size=[mount_x, mount_y*2,mount_z]);
            }
            translate([0,0,board_offset*4]) {
                cube(size=[mount_x, mount_y*2,mount_z]);
            }
        }
        translate([mount_x/2,mount_y+hole_y_offset+2,0]) {
            cylinder(h=mount_extension+board_z+mount_z+(board_offset*4) , r=(hole/2),,$fn=30);
        }
    }
}

module lcd_mount() {
    cube(size=[10,60,10]);
    rotate([45, 0, 0]) {
        difference() {
            cube(size=[10,70,10]);
            translate([5,5,0]) {
            cylinder(h=mount_extension+board_z+mount_z+(board_offset*4) , 1.7, $fn=30);
            }
            translate([5,60,0]) {
            cylinder(h=10 , r=1.7, $fn=30);
            }
        }
    } 
}

//boards();
mount();
//lcd_mount();