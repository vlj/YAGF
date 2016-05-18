using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

        public string HelloWorld(string name)
        {
            return "Hello " + name;
        }
    }
}
