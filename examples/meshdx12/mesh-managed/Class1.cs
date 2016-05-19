using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace mesh_managed
{

    public sealed class CustomAppDomainManager : AppDomainManager, Interface1
    {
        public CustomAppDomainManager()
        {

        }

        public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
        {
            this.InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
        }

        [STAThread]
        public string HelloWorld(string name)
        {
            var tmp = new YAGFControl();
            tmp.Title = "truc";
            tmp.Show();
            return "Hello " + name;
        }
    }
}
