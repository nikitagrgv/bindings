#include <QApplication>
#include <QFrame>
#include <QMainWindow>
#include <QMetaObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVector>

#include <memory>

class BindingBase : public QObject
{
public:
	virtual ~BindingBase() = default;

	virtual void update() = 0;

	virtual void attach(QWidget *widget) = 0;
};


template<class T>
class BindingTemplate : public BindingBase
{
public:
	using Getter = std::function<T()>;
	using Setter = std::function<void(T)>;

	class View
	{
	public:
		View() = default;

		virtual ~View() = default;

		virtual void update() = 0;
	};

public:
	BindingTemplate(Getter getter, Setter setter)
		: getter_(std::move(getter))
		, setter_(std::move(setter))
	{}

	~BindingTemplate() { qDeleteAll(views_); }

	void attach(QWidget *widget) override;

	T get() { return getter_(); }

	void set(T value)
	{
		setter_(value);
		update();
	}

	void update() override
	{
		for (const auto &view : views_)
		{
			view->update();
		}
	}

private:
	QVector<View *> views_;

	Getter getter_;
	Setter setter_;
};

class SliderView : public BindingTemplate<int>::View
{
public:
	SliderView(QSlider *slider, BindingTemplate<int> &binding)
		: slider_(slider)
		, binding_(binding)
	{
		connections_.append(QObject::connect(slider_, &QSlider::valueChanged,
			[this](int value) { binding_.set(value); }));
	}

	~SliderView()
	{
		for (const auto &connection : connections_)
		{
			QObject::disconnect(connection);
		}
	}

	void update() override
	{
		const int value = binding_.get();
		slider_->setValue(value);
	}

private:
	QVector<QMetaObject::Connection> connections_;

	QSlider *slider_{};
	BindingTemplate<int> &binding_;
};

class SpinBoxView : public BindingTemplate<int>::View
{
public:
	SpinBoxView(QSpinBox *spin_box, BindingTemplate<int> &binding)
		: spin_box_(spin_box)
		, binding_(binding)
	{
		QObject::connect(spin_box_, QOverload<int>::of(&QSpinBox::valueChanged),
			[this](int value) { binding_.set(value); });
	}

	~SpinBoxView()
	{
		for (const auto &connection : connections_)
		{
			QObject::disconnect(connection);
		}
	}

	void update() override
	{
		const int value = binding_.get();
		spin_box_->setValue(value);
	}

private:
	QVector<QMetaObject::Connection> connections_;

	QSpinBox *spin_box_{};
	BindingTemplate<int> &binding_;
};

template<class T>
void BindingTemplate<T>::attach(QWidget *widget)
{
	if (auto slider = qobject_cast<QSlider *>(widget))
	{
		views_.append(new SliderView(slider, *this));
	}
	else if (auto spinbox = qobject_cast<QSpinBox *>(widget))
	{
		views_.append(new SpinBoxView(spinbox, *this));
	}
	else
	{
		assert(0);
	}
}

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

class Binder
{
public:
	template<class T>
	BindingBase *createBinding(std::function<T()> getter, std::function<void(T)> setter)
	{
		std::unique_ptr<BindingBase> binding
			= std::make_unique<BindingTemplate<T>>(std::move(getter), std::move(setter));
		BindingBase *ptr = binding.get();
		bindings_.push_back(std::move(binding));
		return ptr;
	}

private:
	std::vector<std::unique_ptr<BindingBase>> bindings_;
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

		auto binding = binder_.createBinding<int>([this]() { return value_; },
			[this](int value) { value_ = value; });

		{
			auto *container = new QWidget(this);
			sliders->layout()->addWidget(container);
			new QHBoxLayout(container);

			auto spinbox1 = new QSpinBox(this);
			container->layout()->addWidget(spinbox1);
			binding->attach(spinbox1);

			auto slider1 = new QSlider(Qt::Horizontal, this);
			container->layout()->addWidget(slider1);
			binding->attach(slider1);
		}

		{
			auto *container = new QWidget(this);
			sliders->layout()->addWidget(container);
			new QHBoxLayout(container);

			auto spinbox2 = new QSpinBox(this);
			container->layout()->addWidget(spinbox2);
			binding->attach(spinbox2);

			auto slider2 = new QSlider(Qt::Horizontal, this);
			container->layout()->addWidget(slider2);
			binding->attach(slider2);
		}
		layout->setStretchFactor(frame, 1);
		layout->setStretchFactor(sliders, 0);
	}

private:
	Binder binder_;
	Canvas *canvas_{};
	int value_ = 50;
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
