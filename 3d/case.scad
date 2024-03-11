include <BOSL2/std.scad>

$fn=100;
OVERLAP = 0.1;

WALL_SIDE = 1.8;
WALL_AROUND_INSERT = 1.2;
WALL_BOTTOM = 1.5;
WALL_TOP = 1.5;
WALL_tol = 0.25;

EXTRA_SPACE_TOP_Y = 5.8;  // needed to insert PCB sandwitch in an angled motion

PCB_X = 113.1;
PCB_Y = 44.7;
PCB_Z = 1.6;
PCB_MNT_HOLE_EDGE_X = 3.2;  // from outer PCB edge
PCB_MNT_HOLE_EDGE_Y = [PCB_Y/2 - 9.3, -PCB_Y/2 + 9.3];  // from outer PCB edge
PCB_MNT_HOLE_MID_X = -PCB_X/2 + 61;  // from left PCB edge
PCB_MNT_HOLE_MID_Y = PCB_Y/2 - 3.3;  // from outer PCB edge
PCB_COMPONENTS_TOP = 18 + 3.4;  // account for hacky insert above audio jack
PCB_COMPONENTS_BOT = 8;
PCB_MOUNTS_TOL = 0.15;

POWER_IN_CENTER_OFF_X = 11.105;  // from left PCB edge
POWER_IN_X = 8.81 + 0.5;
POWER_IN_Z = 7 + 0.8;
POWER_IN_CENTER_OFF_Z = POWER_IN_Z/2 - PCB_MOUNTS_TOL - 0.1;

POWER_OUT_CENTER_OFF_X = 27;  // from left PCB edge
POWER_OUT_SPACING = 19.1;
POWER_OUT_NUM = 2;
POWER_OUT_X = 7.1*2 + 1 + 0.5;
POWER_OUT_Z = 10;
POWER_OUT_CENTER_OFF_Z = POWER_OUT_Z/2 - PCB_MOUNTS_TOL - 0.1;

CON_BTN_CENTER_OFF_X = 68.85;  // from left PCB edge
CON_BTN_X = 22.4 + 0.5;
CON_BTN_Z = 8.8 + 0.5;
CON_BTN_CENTER_OFF_Z = CON_BTN_Z/2 - PCB_MOUNTS_TOL - 0.1;

CON_USB_CENTER_OFF_X = 89.1;  // from left PCB edge
CON_USB_X = 15 + 0.5;
CON_USB_Z = 7.6 + 0.5;
CON_USB_CENTER_OFF_Z = -PCB_Z - CON_USB_Z/2 - PCB_MOUNTS_TOL + 0.1;

CON_AUDIO_CENTER_OFF_X = 108;  // from left PCB edge
CON_AUDIO_D = 6.5;
CON_AUDIO_X = CON_AUDIO_D + 0.1;
CON_AUDIO_Z = CON_AUDIO_X;
CON_AUDIO_CENTER_OFF_Z = 13 - PCB_MOUNTS_TOL - 0.1;

M3_HOLE_D = 3.4;
INSERT_M3_HOLE_D = 4.01 + 0.3;
INSERT_M3_D = 4.55;
INSERT_HEIGHT = 5.8;
INSERT_DEPTH_CASE = 15;
INSERT_DEPTH_PCB = INSERT_HEIGHT + 0.5;

// outside dimensions
CASE_EXTRA_X_LID_MOUNTS = 0;//(INSERT_M3_D + WALL_AROUND_INSERT)*2;
CASE_X = 2* WALL_SIDE + 2* WALL_tol + PCB_X + CASE_EXTRA_X_LID_MOUNTS;
CASE_Y = 2* WALL_SIDE + 2* WALL_tol + PCB_Y + EXTRA_SPACE_TOP_Y;
CASE_BOT_Z = WALL_BOTTOM + 2* WALL_tol + max(INSERT_DEPTH_PCB, PCB_COMPONENTS_BOT) + PCB_Z + PCB_COMPONENTS_TOP;
CASE_BOT_OFF_Z = -WALL_BOTTOM - WALL_tol - max(INSERT_DEPTH_PCB, PCB_COMPONENTS_BOT);  // bottom of case to bottom of PCB
CASE_ROUNDING_R = 3;
CASE_ROUNDING_INNER_R = 3;

TOP_reinforcement_xy = 1.6;
TOP_reinforcement_z = 1.6;

module dummy()
{
  // PCB
  translate([-PCB_X/2, PCB_Y/2, 1.57252/2]) rotate([0, 0, 0]) import("pcb.stl");
}

module cutout_front(dim_x, dim_z, round_r, offset_center_x, offset_center_z)
{
  translate([-PCB_X/2 + offset_center_x, -CASE_Y/2 + WALL_SIDE/2, -CASE_BOT_OFF_Z + PCB_Z + offset_center_z])
  {
    cuboid([dim_x, CASE_Y/2, dim_z], rounding=round_r, edges="Y", anchor=CENTER);
  }
}

module case_bottom()
{
  translate([0, EXTRA_SPACE_TOP_Y/2, CASE_BOT_OFF_Z])
  {
    difference()
    {
      // outer box
      cuboid([CASE_X, CASE_Y, CASE_BOT_Z], rounding=CASE_ROUNDING_R, edges="Z", anchor=BOTTOM);
      
      // inner cutout
      translate([0, 0, WALL_BOTTOM])
      difference()
      {
        // wide sides of PCB space plus extra on top
        cuboid([PCB_X + WALL_tol*2 + CASE_EXTRA_X_LID_MOUNTS, PCB_Y + EXTRA_SPACE_TOP_Y + WALL_tol*2, 1000], 
               rounding=CASE_ROUNDING_INNER_R, anchor=BOTTOM);
        
        // inner corners with M3 inserts
        // round side
        xcopies(spacing=CASE_X - WALL_SIDE*2 - INSERT_M3_D, n=2)
        ycopies(spacing=CASE_Y - WALL_SIDE*2 - INSERT_M3_D, n=2)
          cyl(h=1000, r=INSERT_M3_D/2 + WALL_AROUND_INSERT, center=false);
        
        // flat sides of corners
        xcopies(spacing=CASE_X - WALL_SIDE*2, n=2)
        ycopies(spacing=CASE_Y - WALL_SIDE*2, n=2)
        {
          cube([(INSERT_M3_D), (INSERT_M3_D + WALL_AROUND_INSERT)*2, 1000], center=true);
          cube([(INSERT_M3_D + WALL_AROUND_INSERT)*2, (INSERT_M3_D), 1000], center=true);
        }
        
        // PCB standoffs with inserts
        translate([0, -EXTRA_SPACE_TOP_Y/2, -CASE_BOT_OFF_Z - WALL_BOTTOM - PCB_MOUNTS_TOL])  // move just slightly under PCB
        {
          mirror_copy([1, 0, 0], offset=(PCB_X - PCB_MNT_HOLE_EDGE_X*2)/2)
            ycopies(PCB_MNT_HOLE_EDGE_Y)
              difference()
              {
                union() {
                  cyl(h=1000, d=INSERT_M3_D + WALL_AROUND_INSERT*2, anchor=TOP);
                  cube([10, INSERT_M3_D + WALL_AROUND_INSERT*2, 1000], anchor=TOP+LEFT);
                }
                translate([0, 0, OVERLAP])
                  cyl(h=INSERT_DEPTH_PCB + OVERLAP, d=INSERT_M3_HOLE_D, anchor=TOP);
              }
          
          // middle standoff
          translate([PCB_MNT_HOLE_MID_X, PCB_MNT_HOLE_MID_Y, 0])
            difference()
            {
              cyl(h=1000, d=INSERT_M3_D + WALL_AROUND_INSERT*2, anchor=TOP);
              translate([0, 0, OVERLAP])
                cyl(h=INSERT_DEPTH_PCB + OVERLAP, d=INSERT_M3_HOLE_D, anchor=TOP);
            } 
        }
      }
      
      // hack to make room for badly placed audio module
      translate([CASE_X/2 - WALL_SIDE - INSERT_M3_D/2 - WALL_AROUND_INSERT/2 - OVERLAP/2,
                  -CASE_Y/2 + WALL_SIDE + INSERT_M3_D/2 + WALL_AROUND_INSERT/2 + OVERLAP/2,
                  -CASE_BOT_OFF_Z])
        cube([INSERT_M3_D + WALL_AROUND_INSERT + OVERLAP, INSERT_M3_D + WALL_AROUND_INSERT + OVERLAP, CON_AUDIO_CENTER_OFF_Z+2], anchor=BOT);
      
      // M3 inserts to mount case top lid
      translate([0, 0, CASE_BOT_Z + OVERLAP])
        xcopies(spacing=CASE_X - WALL_SIDE*2 - INSERT_M3_D, n=2)
        ycopies(spacing=CASE_Y - WALL_SIDE*2 - INSERT_M3_D, n=2)
          cyl(h=INSERT_DEPTH_CASE + OVERLAP, d=INSERT_M3_HOLE_D, anchor=TOP);
      
      // power connector
      cutout_front(POWER_IN_X, POWER_IN_Z, round_r=0, offset_center_x=POWER_IN_CENTER_OFF_X, offset_center_z=POWER_IN_CENTER_OFF_Z);
      
      // power connector label
      text_depth = 0.5;  // imprint depth on sides
      text_h = text_depth + OVERLAP;  // height of subtractor
      translate([-PCB_X/2 + POWER_IN_CENTER_OFF_X, 
                  -CASE_Y/2 + text_depth/2, 
                  -CASE_BOT_OFF_Z + PCB_Z])
      {
        translate([-2, 0, -3]) rotate([90,0,0])
          linear_extrude(height=text_h, center=true)
            text("+", size=5, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
                 
        translate([2, 0, -3]) rotate([90,0,0])
          linear_extrude(height=text_h, center=true)
            text("-", size=5, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
      }
      
      // power out connectors
      xcopies(spacing=POWER_OUT_SPACING, n=POWER_OUT_NUM, sp=0)
      {
        cutout_front(POWER_OUT_X, POWER_OUT_Z, round_r=0, offset_center_x=POWER_OUT_CENTER_OFF_X, offset_center_z=POWER_OUT_CENTER_OFF_Z);
        
        translate([-PCB_X/2 + POWER_OUT_CENTER_OFF_X, 
                    -CASE_Y/2 + text_depth/2, 
                    -CASE_BOT_OFF_Z + PCB_Z])
        {
          translate([-5.8, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
              text("+", size=4, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
          translate([-2.3, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
              text("-", size=4, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
          translate([5.8, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
              text("-", size=4, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
          translate([2.3, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
              text("+", size=4, font="Liberation Sans:style=Bold", halign="center", valign="center", $fn=16);
        }
      }
      
      // button connectors
      cutout_front(CON_BTN_X, CON_BTN_Z, round_r=0, offset_center_x=CON_BTN_CENTER_OFF_X, offset_center_z=CON_BTN_CENTER_OFF_Z);
      
      translate([-PCB_X/2 + CON_BTN_CENTER_OFF_X, 
                  -CASE_Y/2 + text_depth/2, 
                  -CASE_BOT_OFF_Z + PCB_Z])
      {
        translate([-5.8, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
            text("A-A-", size=3, font="Liberation Mono:style=Bold", halign="center", valign="center", $fn=16);
        translate([5.8, 0, -3]) rotate([90,0,0]) linear_extrude(height=text_h, center=true)
            text("B-B-", size=3, font="Liberation Mono:style=Bold", halign="center", valign="center", $fn=16);
      }
      
      // USB power out
      cutout_front(CON_USB_X, CON_USB_Z, round_r=0, offset_center_x=CON_USB_CENTER_OFF_X, offset_center_z=CON_USB_CENTER_OFF_Z);
      
      // line out
      cutout_front(CON_AUDIO_X, CON_AUDIO_Z, round_r=CON_AUDIO_D/2, offset_center_x=CON_AUDIO_CENTER_OFF_X, offset_center_z=CON_AUDIO_CENTER_OFF_Z);
    }
  }
}


module case_top()
{
  translate([0, EXTRA_SPACE_TOP_Y/2, 0])
  {
    difference()
    {
      cuboid([CASE_X, CASE_Y, WALL_TOP], rounding=CASE_ROUNDING_R, edges="Z", anchor=BOTTOM);
      
      // holes for screws
      translate([0, 0, -OVERLAP])
        xcopies(spacing=CASE_X - WALL_SIDE*2 - INSERT_M3_D, n=2)
        ycopies(spacing=CASE_Y - WALL_SIDE*2 - INSERT_M3_D, n=2)
          cyl(h=WALL_TOP + OVERLAP*2, d=M3_HOLE_D, anchor=BOTTOM);
      
      text_depth = 0.2;  // imprint depth
      text_h = text_depth + OVERLAP;  // height of subtractor
      
      // label
      translate([0, 0, WALL_TOP + text_h/2 - text_depth])
        linear_extrude(height=text_h, center=true)
          text("Wakeuplight v2", size=7, font="Liberation Sans:style=Bold Italic", 
               halign="center", valign="center", $fn=16);
    }
      
    // ledge all around for stability
    translate([0, 0, OVERLAP])
    difference()
    {
      rect_tube(size=[CASE_X - WALL_SIDE*2 - 0.3, CASE_Y - WALL_SIDE*2 - 0.3], wall=TOP_reinforcement_xy, 
                rounding=CASE_ROUNDING_R, irounding=CASE_ROUNDING_R-TOP_reinforcement_xy, 
                h=TOP_reinforcement_z + OVERLAP, anchor=TOP);
    
      // cut out edges / move around
      translate([0, 0, 0])
        xcopies(spacing=CASE_X - WALL_SIDE*2 - INSERT_M3_D - WALL_AROUND_INSERT, n=2)
        ycopies(spacing=CASE_Y - WALL_SIDE*2 - INSERT_M3_D - WALL_AROUND_INSERT, n=2)
          cube([INSERT_M3_D + WALL_AROUND_INSERT + WALL_tol, INSERT_M3_D + WALL_AROUND_INSERT + WALL_tol, 10], 
               anchor=TOP);
    }
  }
}


//translate([0, EXTRA_SPACE_TOP_Y, 0])  // when inserting
%dummy();

difference()
{
  case_bottom();
  // debug:
//  translate([0, 0, 0]) cube([200, 200, 200], anchor=RIGHT+BACK); // +FRONT+BACK
//  translate([35, 0, 0]) cube([200, 200, 200], anchor=LEFT+BACK);
}

translate([0, 0, CASE_BOT_OFF_Z + CASE_BOT_Z + 0.1])
difference()
{
  case_top();
  // debug:
//  cube([200, 200, 200], anchor=RIGHT+FRONT);
}
