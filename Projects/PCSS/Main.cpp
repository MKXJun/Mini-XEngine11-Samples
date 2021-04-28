#include "Renderer.h"
 
int main() try
{
	// 允许在Debug版本进行运行时内存分配和泄漏检测
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	Renderer renderer("DirectX11 Test", 1280, 720);
	
	Scene scene;

	scene.AddCube("Cube");

	while (renderer.IsAlive())
	{
		renderer.Render(scene);

		renderer.EndFrame();
	}

	std::tuple<double, char, std::string> t0{ 2.4, 'a', "Hello" };
	auto t1 = std::make_tuple(2.4, 'a', std::string("Hello"));

	return 0;
}
catch (std::exception& e)
{

}
catch (DxException& e)
{
	MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
}


