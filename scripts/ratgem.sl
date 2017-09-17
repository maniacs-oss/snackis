struct: Point
  x y Num;

func: point(x y Num)
  Point new $ @x @y init;

func: init(p Point x y Num)
  @p @x set-x
  @p @y set-y;

struct: Line Point
  z Num;

func: init(l Line x y z Num)
  @l @x @y init<Point>
  @l @z set-z;

func: line(x y z Num)
  Line new $ @x @y @z init;

func: tests
  1 2 3 line;