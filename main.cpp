#include <QApplication>
#include <QFrame>
#include <QMainWindow>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>


class Canvas : public QWidget
{
public:
	explicit Canvas(QWidget *parent = nullptr)
		: QWidget(parent)
	{}

	void paintEvent(QPaintEvent *event) override
	{
		QPainter painter(this);
		const QRect rect = event->rect();
		painter.drawLine(rect.center(), rect.bottomRight());
	}
};


class MainWidget : public QWidget
{
public:
	explicit MainWidget(QWidget *parent = nullptr)
		: QWidget(parent)
	{
		auto *layout = new QHBoxLayout(this);

		auto *frame = new QFrame(this);
		frame->setFrameShadow(QFrame::Sunken);
		frame->setFrameShape(QFrame::Box);
		new QHBoxLayout(frame);
		layout->addWidget(frame);

		canvas_ = new Canvas(this);
		frame->layout()->addWidget(canvas_);

		auto *sliders = new QWidget(this);
		new QVBoxLayout(sliders);
		layout->addWidget(sliders);

		auto slider1 = new QSlider(Qt::Horizontal, this);
		sliders->layout()->addWidget(slider1);

		auto slider2 = new QSlider(Qt::Horizontal, this);
		sliders->layout()->addWidget(slider2);


		layout->setStretchFactor(frame, 1);
		layout->setStretchFactor(sliders, 0);
	}

private:
	Canvas *canvas_{};
};

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QMainWindow window;
	auto *main_widget = new MainWidget(&window);
	window.setCentralWidget(main_widget);
	window.resize(500, 500);
	window.show();
	return QApplication::exec();
}
