#pragma once

namespace SP_LAB
{
	class Sandbox
	{
	public:
		Sandbox();
		virtual ~Sandbox() {}
		virtual void Load() {}
		virtual void Draw() {}
	};
}