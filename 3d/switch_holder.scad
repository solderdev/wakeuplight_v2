include <BOSL2/std.scad>
//use <cherrymx.scad>

$fn=100;

wall = 1.0;
cutout_sidelen = 14;
switch_height = 9;  // below surface
rounding_sides = 1;
indent_width = 5;
indent_height = 2;
indent_offset_top_z = 2;
indent_depth = 0.5; // into wall

cable_height = 2;
cable_width = 4;
cable_tol = 0.15;

module hole_cable(orientation)
{
  hull()
  {
    translate([(cable_width - cable_height)/2, 0, cable_height/2 + cable_tol/2])
      cylinder(h=cutout_sidelen+wall, d=cable_height+cable_tol, orient=orientation);
    translate([-(cable_width - cable_height)/2, 0, cable_height/2 + cable_tol/2])
      cylinder(h=cutout_sidelen+wall, d=cable_height+cable_tol, orient=orientation);
  }
}

difference()
{
  // outer shell
  cuboid([cutout_sidelen + wall*2, cutout_sidelen + wall*2, switch_height + wall], rounding=rounding_sides, edges=["Z"], anchor=BOT);
  
  // cutout for switch
  translate([0, 0, wall])
    cube([cutout_sidelen, cutout_sidelen, switch_height + 0.1], anchor=BOT);
    
  // indents on sides for switch to clip in
  ycopies(cutout_sidelen - indent_height * sqrt(2) + indent_depth*2)
    translate([0, 0, wall + switch_height - indent_offset_top_z])
      xrot(45)
        cube([indent_width, indent_height, indent_height], center=true);
  
  // hole for cable in back of case
  translate([0, 0, wall])
    hole_cable(BACK);
    
  // hole for cable in bottom of case
  translate([0, cutout_sidelen/2 - (cable_height/2 + cable_tol/2), wall])
    hole_cable(DOWN);
  
  // debug
//  cube(100);
}

//*translate([0, 0, 23.4])
//  #CherryMX();