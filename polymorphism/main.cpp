#include <iostream>
#include <memory>
#include <vector>
#include <string>

struct Shape {
    virtual ~Shape() = default;               // polymorphic base needs virtual dtor
    virtual double area() const = 0;          // virtual abstract
    virtual void draw() const = 0;
};

struct Circle : Shape {
    double r;
    explicit Circle(double radius) : r(radius) {}
    double area() const override { return 3.14159 * r * r; }
    void draw() const override { std::cout << "  circle r=" << r << "\n"; }
};

struct Rectangle : Shape {
    double w, h;
    Rectangle(double width, double height) : w(width), h(height) {}
    double area() const override { return w * h; }
    void draw() const override { std::cout << "  rect " << w << "x" << h << "\n"; }
};

void print_area(const Shape& s) {
    std::cout << "  area = " << s.area() << "\n";
}

int main() {
    std::cout << "static\n";
    Circle c(2.0);
    Rectangle r(3.0, 4.0);
    c.draw();
    r.draw();

    std::cout << "polymorphic via reference\n";
    print_area(c);
    print_area(r);

    std::cout << "polymorphic via unique_ptr + vector\n";
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(1.5));
    shapes.push_back(std::make_unique<Rectangle>(2.0, 5.0));
    shapes.push_back(std::make_unique<Circle>(3.0));

    for (const auto& s : shapes) {
        s->draw();
        print_area(*s);
    }
}