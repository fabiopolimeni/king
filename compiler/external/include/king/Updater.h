// (C) king.com Ltd 2014

#pragma once

namespace King
{
	struct Input;
	class Updater
	{
	public:

		virtual bool Init() = 0;
		virtual void Update() = 0;

	protected:

		~Updater() {}
	};
}
