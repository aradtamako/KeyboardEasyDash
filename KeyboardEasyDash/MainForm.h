#pragma once

#include "VirtualGamePadState.hpp"

namespace KeyboardEasyDash {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	PVIGEM_TARGET InitializeVirtualGamePad();

	/// <summary>
	/// MainForm の概要
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			//
			//TODO: ここにコンストラクター コードを追加します
			//
		}

	protected:
		/// <summary>
		/// 使用中のリソースをすべてクリーンアップします。
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::RadioButton^ radioButtonPotionEnable;
	private: System::Windows::Forms::RadioButton^ radioButtonPotionDisable;

	private:
		/// <summary>
		/// 必要なデザイナー変数です。
		/// </summary>
		System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// デザイナー サポートに必要なメソッドです。このメソッドの内容を
		/// コード エディターで変更しないでください。
		/// </summary>
		void InitializeComponent(void)
		{
			this->radioButtonPotionEnable = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonPotionDisable = (gcnew System::Windows::Forms::RadioButton());
			this->SuspendLayout();
			// 
			// radioButtonPotionEnable
			// 
			this->radioButtonPotionEnable->AutoSize = true;
			this->radioButtonPotionEnable->Checked = true;
			this->radioButtonPotionEnable->Location = System::Drawing::Point(12, 34);
			this->radioButtonPotionEnable->Name = L"radioButtonPotionEnable";
			this->radioButtonPotionEnable->Size = System::Drawing::Size(47, 16);
			this->radioButtonPotionEnable->TabIndex = 1;
			this->radioButtonPotionEnable->TabStop = true;
			this->radioButtonPotionEnable->Text = L"有効";
			this->radioButtonPotionEnable->UseVisualStyleBackColor = true;
			this->radioButtonPotionEnable->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButtonPotionEnable_CheckedChanged);
			// 
			// radioButtonPotionDisable
			// 
			this->radioButtonPotionDisable->AutoSize = true;
			this->radioButtonPotionDisable->Location = System::Drawing::Point(12, 12);
			this->radioButtonPotionDisable->Name = L"radioButtonPotionDisable";
			this->radioButtonPotionDisable->Size = System::Drawing::Size(47, 16);
			this->radioButtonPotionDisable->TabIndex = 0;
			this->radioButtonPotionDisable->Text = L"無効";
			this->radioButtonPotionDisable->UseVisualStyleBackColor = true;
			this->radioButtonPotionDisable->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButtonPotionDisable_CheckedChanged);
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(84, 60);
			this->Controls->Add(this->radioButtonPotionEnable);
			this->Controls->Add(this->radioButtonPotionDisable);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
			this->Name = L"MainForm";
			this->Text = L"KeyboardEasyDash";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MainForm::d);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private: System::Void radioButtonPotionDisable_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
		if (radioButtonPotionDisable->Checked) {
			vigem_target_remove(VirtualGamePadState::Client, VirtualGamePadState::Target);
		}
	}

	private: System::Void radioButtonPotionEnable_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
		if (radioButtonPotionEnable->Checked) {
			InitializeVirtualGamePad();
		}
	}

	private: System::Void d(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e) {
		e->Cancel = true;
	}

	};
}
