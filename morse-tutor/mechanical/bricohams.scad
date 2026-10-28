// Original Bricohams prototype, millimetres. Measure your board before printing.
product = "clock"; // "clock" or "tutor"
part = "base"; // base, lid, paddle_left, paddle_right, clamp, fit_coupon
$fn = 40;
family = false; // Export tool supplies measured/design envelopes for each variant.
wall = 2.4;
floor_thickness = 3;
clearance = 0.6;
pcb_w = 86;
pcb_h = 50;
screen_w = 70;
screen_h = 43;
screen_stack = 9;
bank_w = 100;
bank_h = 60;
bank_t = 22;
is_tutor = product == "tutor";
case_w = family ? max(is_tutor ? 174 : 116, pcb_w+28, bank_w+16) : (is_tutor ? 174 : 116);
case_h = family ? max(pcb_h+24,bank_h+20)+(is_tutor ? 56 : 0) : (is_tutor ? 132 : 90);
case_z = family ? max(is_tutor ? 60 : 48,screen_stack+bank_t+14) : (is_tutor ? 60 : 48);
rear_start = is_tutor ? 56 : 0;
board_x = case_w / 2;
board_y = rear_start+(case_h-rear_start)/2;
board_z = case_z - screen_stack - 1;
lid_t = 2.4;
lip = 1.5;

assert(pcb_w + 2 * clearance < case_w - 2 * wall - 12);
assert(pcb_h + 2 * clearance < case_h - rear_start - 2 * wall - 8);
assert(bank_w + 2 * clearance < case_w - 2 * wall);
assert(bank_h + 2 * clearance < case_h - rear_start - 2 * wall);
assert(bank_t + floor_thickness + 5 < board_z);
assert(screen_w < pcb_w && screen_h < pcb_h);
assert(screen_w+2*wall < case_w && screen_h+2*wall < case_h-rear_start);

module round_box(w, h, z, r=4) {
    hull() for (x=[r,w-r], y=[r,h-r])
        translate([x,y,0]) cylinder(r=r,h=z);
}

module screw_positions() {
    for (x=[6,case_w-6], y=[rear_start+6,case_h-6])
        translate([x,y,0]) children();
}

module board_corners() {
    for (sx=[-1,1], sy=[-1,1])
        translate([board_x+sx*(pcb_w/2-2),
                   board_y+sy*(pcb_h/2-2),0]) children();
}

module rear_shell() {
    difference() {
        translate([0,rear_start,0])
            round_box(case_w,case_h-rear_start,case_z);
        translate([wall,rear_start+wall,floor_thickness])
            round_box(case_w-2*wall,case_h-rear_start-2*wall,case_z,2);
        // Oversized cable apertures: final connector positions are board-specific.
        translate([-1,rear_start+17,10]) cube([wall+2,20,14]);
        translate([case_w-wall-1,rear_start+10,10]) cube([wall+2,20,14]);
        if (is_tutor) for (y=[rear_start+35,rear_start+50])
            translate([case_w-wall-1,y,43])
                rotate([0,90,0]) cylinder(d=6.5,h=wall+2);
        for (i=[0:5])
            translate([25+i*11,case_h-wall-1,8]) cube([6,wall+2,4]);
    }
}

module base() {
    difference() {
        union() {
            rear_shell();
            screw_positions() cylinder(d=8,h=case_z-1);
            // These ledges support PCB corners; no undocumented PCB hole pattern.
            for (sy=[-1,1], sx=[-1,1]) {
                edge_x = sx < 0 ? wall-0.1 : board_x+pcb_w/2-4;
                beam_w = sx < 0 ? board_x-pcb_w/2+4-wall+0.1
                               : case_w-wall-edge_x+0.1;
                translate([edge_x,board_y+sy*(pcb_h/2-2)-2.5,board_z-3])
                    cube([beam_w,5,3]);
            }
            // Retaining rails keep an enclosed USB power bank off the side walls.
            for (s=[-1,1])
                translate([board_x+s*(bank_w/2+clearance)-1,
                           board_y-bank_h/2, floor_thickness])
                    cube([2,bank_h,3]);
            if (is_tutor) {
                round_box(case_w,rear_start+wall,floor_thickness);
                for (x=[60,114])
                    translate([x,43,floor_thickness]) cylinder(d=10,h=5);
                for (x=[74,100])
                    translate([x-3,23-3,floor_thickness]) cube([6,6,14]);
                for (x=[48,126])
                    translate([x,10,floor_thickness]) cylinder(d=5,h=12);
                // Adapter tray pillars, 80 x 40 mm PCB with 3.2 mm corner holes.
                for (y=[77,111])
                    translate([wall-0.1,y-3.5,29]) cube([case_w-2*wall+0.2,7,3]);
                for (x=[50,124], y=[77,111])
                    translate([x,y,29]) cylinder(d=7,h=3);
            }
        }
        screw_positions() translate([0,0,floor_thickness]) cylinder(d=2.7,h=case_z);
        if (is_tutor) {
            for (x=[60,114])
                translate([x,43,-1]) cylinder(d=3.3,h=12);
            for (x=[74,100])
                translate([x-5,23,12]) rotate([0,90,0]) cylinder(d=2.8,h=10);
            for (x=[50,124], y=[77,111])
                translate([x,y,28]) cylinder(d=2.7,h=5);
            for (x=[60,114])
                translate([x,43,-0.01]) cylinder(d=6.4,h=2.5,$fn=6);
            for (x=[60,114])
                translate([x,15,-1]) cylinder(d=3,h=floor_thickness+2);
        }
    }
}

module logo(height=0.6) {
    // Text wordmark, not an imitation of the Morserino logo.
    linear_extrude(height=height)
        text("BRICOHAMS",size=5,font="Liberation Sans:style=Bold",
             halign="center",valign="center");
}

module lid() {
    difference() {
        union() {
            translate([0,rear_start,0]) round_box(case_w,case_h-rear_start,lid_t);
            translate([wall+clearance,rear_start+wall+clearance,lid_t-0.2])
                difference() {
                    round_box(case_w-2*(wall+clearance),
                              case_h-rear_start-2*(wall+clearance),lip+0.2,2);
                    translate([1.5,1.5,-1])
                        round_box(case_w-2*(wall+clearance)-3,
                                  case_h-rear_start-2*(wall+clearance)-3,lip+2,1);
                }
        }
        translate([board_x-screen_w/2,board_y-screen_h/2,-1])
            cube([screen_w,screen_h,lid_t+lip+2]);
        screw_positions() translate([0,0,-1]) cylinder(d=3.4,h=lid_t+lip+2);
        // Face down on the print bed: engraved brand remains printable without support.
        translate([case_w/2,rear_start+10,-0.01]) mirror([1,0,0]) logo(0.7);
    }
}

module paddle(left=true) {
    // Print flat. M3 vertical pivot and horizontal M3 contact screw.
    difference() {
        union() {
            translate([-4,-38,0]) cube([8,38,12]);
            cylinder(d=14,h=12);
            translate([-7,-40,0]) round_box(14,12,16,2);
        }
        translate([0,0,-1]) cylinder(d=3.3,h=15);
        translate([-9,-20,4]) rotate([0,90,0]) cylinder(d=3.3,h=18);
        translate([0,-31,-1]) cylinder(d=2.8,h=19);
        translate([0,-5,11.4])
            linear_extrude(height=1)
                text(left ? "." : "-",size=6,halign="center");
    }
}

module clamp() {
    difference() {
        cube([12,6,3]);
        translate([3,3,-1]) cylinder(d=3.2,h=5);
    }
}

module fit_coupon() {
    difference() {
        round_box(40,25,3,2);
        for (i=[0:3])
            translate([7+i*8,8,-1]) cylinder(d=3+i*0.1,h=5);
        translate([4,16,-1]) cube([20,6.5,5]);
    }
}

if (part == "base") base();
else if (part == "lid") lid();
else if (part == "paddle_left") paddle(true);
else if (part == "paddle_right") paddle(false);
else if (part == "clamp") clamp();
else if (part == "fit_coupon") fit_coupon();
else assert(false,"Unknown part");
