/* SPDX-License-Identifier: MIT */
#include <Application.h>
#include <MessageRunner.h>
#include <View.h>
#include <Window.h>

class TestView : public BView {
public:
	TestView(BRect frame)
		: BView(frame, "HDMI visual test", B_FOLLOW_ALL, B_WILL_DRAW)
	{
		SetViewColor(35, 35, 35);
	}
	void Draw(BRect)
	{
		SetHighColor(255, 255, 255);
		SetFontSize(22);
		DrawString("Intel N100 HDMI hardware validation", BPoint(25, 45));
		SetFontSize(15);
		DrawString("Live RenkuOS desktop output — 1920 x 1080", BPoint(25, 80));
		const rgb_color colors[] = {
			{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 255, 255}
		};
		for (unsigned i = 0; i < 4; i++) {
			SetHighColor(colors[i]);
			FillRect(BRect(25 + i * 160, 110, 170 + i * 160, 240));
		}
	}
};

int main()
{
	BApplication app("application/x-vnd.Renku-HDMI-visual-test");
	BWindow* window = new BWindow(BRect(300, 250, 1000, 540),
		"HDMI validation (closes automatically)", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
	window->AddChild(new TestView(window->Bounds()));
	window->Show();
	BMessage quit(B_QUIT_REQUESTED);
	BMessageRunner timer(BMessenger(&app), &quit, 30000000, 1);
	app.Run();
	return 0;
}
