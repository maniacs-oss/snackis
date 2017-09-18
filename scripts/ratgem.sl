struct: Point
  x y Num;

func: init(p Point x y Num)
  @p @x set-x
  @p @y set-y;

func: point(x y Num)
  Point new $ @x @y init;


func: quad(p1 p2 Point)
  // Returns quadrance of @p1 and @p2.
  @p2 x @p1 x - 2^
  @p2 y @p1 y - 2^ -;


struct: Line Point
  z Num;

func: init(l Line x y z Num)
  @l @x @y init<Point>
  @l @z set-z;

func: line(x y z Num)
  Line new $ @x @y @z init;

test: 'lines should be equal to their points'
  1 2 point 1 2 3 line ==;


func: para(l1 l2 Line)
  // Returns true when @l1 and @l2 are parallel.
  @l1 x @l2 y *
  @l1 y @l2 x * - z?;

test: 'lines should be parallel to themselves'
  1 2 3 line 1 2 3 line para;

test: 'lines should be parallel to themselves'
  1 2 3 line 1 2 3 line para;

test: 'lines should be parallel despite offset'
  1 2 3 line 1 2 4 line para;

test: 'lines should not be parallel if x/y differs'
  1 2 3 line 2 3 4 line para~;


func: perp(l1 l2 Line)
  // Returns true when @l1 and @l2 are perpendicular.
  @l1 x @l1 x *
  @l2 y @l2 x * + z?;

test: 'lines should not be perpendicular to themselves'
  1 2 3 line 1 2 3 line perp~;

test: 'lines should be perpendicular if reversed x/y'
  0 1 0 line 1 0 0 line perp;

test: 'lines should be perpendicular despite offset'
  0 1 2 line 1 0 3 line perp;

test: 'lines should not be perpendicular unless reversed x/y'
  0 1 2 line 1 1 2 line perp~;