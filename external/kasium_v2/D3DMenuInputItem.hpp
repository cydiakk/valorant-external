#pragma once
#include "items/ID3DMenuItem.hpp"


#include "stdafx.hpp"


void get_inputkey() {
	bool flag = false;

	while (true) {
		for (int i = 1; i < 0xFD; i++) {
			if ((GetKeyState(i) & 0x100) != 0) {
				settings::aimbot::aimkey = i;
				flag = true;
			}
		}

		if (flag)
			break;
	}
}

class D3DMenuInputItem : public ID3DMenuItem
{
public:
	D3DMenuInputItem(const std::string& _name, float& value_ptr, const float _min, const float _max, const float _steps, const bool is_sub_item = false) :
		ID3DMenuItem(_name, is_sub_item), value(value_ptr), min_value(_min), max_value(_max), step_value(_steps) {}

	std::string get_value_text() const override
	{
		std::ostringstream out;
		out << "<" << std::setprecision(2) << value << ">";
		return out.str();
	}

	void handle_left() override
	{
		//value -= step_value;
		//if (value < min_value)
		//	value = min_value;

		value = 0x00;

		CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)get_inputkey, nullptr, NULL, NULL);
	}

	void handle_right() override
	{
		//value += step_value;
		//if (value > max_value)
		//	value = max_value;

		value = 0x00;

		CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)get_inputkey, nullptr, NULL, NULL);
	}

private:
	float& value;
	float min_value;
	float max_value;
	float step_value;
};
