using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace mesh_managed
{
    public class FormTest : Form
    {
        public Button button1;
        public FormTest()
        {
            button1 = new Button();
            button1.Size = new Size(40, 40);
            button1.Location = new Point(30, 30);
            button1.Text = "Click me";
            this.Controls.Add(button1);
        }
    }

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
            var tmp = new FormTest();
            tmp.Show();
            return "Hello " + name;
        }
    }
}
