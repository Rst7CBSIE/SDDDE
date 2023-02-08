#pragma once
#include <vcclr.h>
#include <atlstr.h>
#include "render.h"
#include "renderutils.h"

namespace E3D {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for mainForm
	/// </summary>
	public ref class mainForm : public System::Windows::Forms::Form
	{
	public:
		mainForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			ScreenBitmap = gcnew Bitmap(180*4, 128*4);
			PrintCEdistances();
			this->MouseWheel += gcnew System::Windows::Forms::MouseEventHandler(this, &E3D::mainForm::OnMouseWheel);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~mainForm()
		{
			if (components)
			{
				delete components;
			}
			delete ScreenBitmap;
		}

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>

		Bitmap^ ScreenBitmap;
		IntPtr BitmapArray;
		void PrintCEdistances(void);
		void ProcessMouse(int x, int y, int buttons);
		void ResetMouse(void);
		void FlyByWheel(int move);
		void PrintPosition(void);
		int LockMouse;
		void SetLockMouse(int lock);
		int mouse_buttons;


	private: System::Windows::Forms::Timer^ timer1;
	private: System::Windows::Forms::PictureBox^ pictureBox1;
	private: System::ComponentModel::IContainer^ components;

	private: System::Windows::Forms::Label^ lblCEdistances;
	private: System::Windows::Forms::Label^ lblMouse;
	private: System::Windows::Forms::Label^ lblPosition;



























#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->lblCEdistances = (gcnew System::Windows::Forms::Label());
			this->lblMouse = (gcnew System::Windows::Forms::Label());
			this->lblPosition = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 20;
			this->timer1->Tick += gcnew System::EventHandler(this, &mainForm::timer1_Tick);
			// 
			// pictureBox1
			// 
			this->pictureBox1->Location = System::Drawing::Point(7, 12);
			this->pictureBox1->Margin = System::Windows::Forms::Padding(4);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(1440, 1024);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
			this->pictureBox1->TabIndex = 5;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Click += gcnew System::EventHandler(this, &mainForm::pictureBox1_Click);
			this->pictureBox1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::pictureBox1_MouseDown);
			this->pictureBox1->MouseEnter += gcnew System::EventHandler(this, &mainForm::pictureBox1_MouseEnter);
			this->pictureBox1->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::mainForm_MouseMove);
			this->pictureBox1->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &mainForm::pictureBox1_MouseUp);
			// 
			// lblCEdistances
			// 
			this->lblCEdistances->AutoSize = true;
			this->lblCEdistances->Location = System::Drawing::Point(24, 1075);
			this->lblCEdistances->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->lblCEdistances->MinimumSize = System::Drawing::Size(300, 200);
			this->lblCEdistances->Name = L"lblCEdistances";
			this->lblCEdistances->Size = System::Drawing::Size(300, 200);
			this->lblCEdistances->TabIndex = 7;
			this->lblCEdistances->Text = L"...";
			// 
			// lblMouse
			// 
			this->lblMouse->AutoSize = true;
			this->lblMouse->Location = System::Drawing::Point(1378, 1064);
			this->lblMouse->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->lblMouse->MinimumSize = System::Drawing::Size(300, 50);
			this->lblMouse->Name = L"lblMouse";
			this->lblMouse->Size = System::Drawing::Size(300, 50);
			this->lblMouse->TabIndex = 8;
			this->lblMouse->Text = L"....";
			// 
			// lblPosition
			// 
			this->lblPosition->AutoSize = true;
			this->lblPosition->Location = System::Drawing::Point(939, 1075);
			this->lblPosition->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->lblPosition->Name = L"lblPosition";
			this->lblPosition->Size = System::Drawing::Size(123, 25);
			this->lblPosition->TabIndex = 9;
			this->lblPosition->Text = L"...position...";
			// 
			// mainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(12, 25);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSize = true;
			this->ClientSize = System::Drawing::Size(1970, 1464);
			this->Controls->Add(this->lblPosition);
			this->Controls->Add(this->lblMouse);
			this->Controls->Add(this->lblCEdistances);
			this->Controls->Add(this->pictureBox1);
			this->KeyPreview = true;
			this->Margin = System::Windows::Forms::Padding(4);
			this->Name = L"mainForm";
			this->Padding = System::Windows::Forms::Padding(20, 19, 20, 19);
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"mainForm";
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &mainForm::mainForm_KeyDown);
			this->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &mainForm::mainForm_KeyPress);
			this->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &mainForm::mainForm_KeyUp);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
		private: System::Void timer1_Tick(System::Object^ sender, System::EventArgs^ e) 
		{
			TimerProc();
		}
		private: System::Void sbPitch_Scroll(System::Object^ sender, System::Windows::Forms::ScrollEventArgs^ e) 
		{
		}
		private: System::Void sbYaw_Scroll(System::Object^ sender, System::Windows::Forms::ScrollEventArgs^ e) 
		{
		}
		private: void TimerProc(void);
		private: void ProcessKeyPress(int ch, int f, int _shift, int _ctrl, int _alt);

		private: System::Void mainForm_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
		{
			//ProcessKeyPress(e->KeyChar);
		}
		private: System::Void mainForm_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) 
		{
			
			ProcessKeyPress(e->KeyValue,1,e->Shift,e->Control,e->Alt);
		}
		private: System::Void mainForm_KeyUp(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) 
		{
			ProcessKeyPress(e->KeyValue, 0,e->Shift,e->Control,e->Alt);
		}
		private: System::Void mainForm_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
		{
			ProcessMouse(e->X, e->Y, mouse_buttons);
		}

		private: System::Void pictureBox1_MouseEnter(System::Object^ sender, System::EventArgs^ e) 
		{
			ResetMouse();
		}
		private: System::Void pictureBox1_Click(System::Object^ sender, System::EventArgs^ e) 
		{
			if (mouse_buttons & 1)
			{
				SetLockMouse(!LockMouse);
			}
			else if (mouse_buttons & 2)
			{
				//SelectFaceIn3D();
			}
		}
		private: System::Void OnMouseWheel(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
		{
			int move = e->Delta;
			FlyByWheel(move);
		}
		private: System::Void pictureBox1_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) 
		{
			switch (e->Button)
			{
			case System::Windows::Forms::MouseButtons::Left:
				mouse_buttons |= 1;
				break;
			case System::Windows::Forms::MouseButtons::Right:
				mouse_buttons |= 2;
				break;
			}
		}
		private: System::Void pictureBox1_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) 
		{
			switch (e->Button)
			{
			case System::Windows::Forms::MouseButtons::Left:
				mouse_buttons &= ~1;
				break;
			case System::Windows::Forms::MouseButtons::Right:
				mouse_buttons &= ~2;
				break;
			}
		}

	};
}
