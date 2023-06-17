import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
    title: "Aerionuaserver",
    description: "Aerion - a OPC-UA Server for MELFA Robots and MELSEC PLCs",
    base: '/aerionuaserver/',
    lastUpdated: true,
    themeConfig: {
        // https://vitepress.dev/reference/default-theme-config
        nav: [
            { text: 'Home', link: '/' },
            { text: 'Getting started', link: '/introduction' }
        ],

        sidebar: [
            {
                text: 'Introduction',
                items: [
                    {
                        text: 'What is aerionuaserver?',
                        link: '/introduction',
                    },
                    {
                        text: 'Installation',
                        link: '/installation'
                    }
                ],
                collapsed: false,
            },
            {
                text: 'Configuration',
                items: [
                    {
                        text: 'Adding a robot',
                        link: '/robot'
                    },
                    {
                        text: 'Adding a plc',
                        link: '/plc'
                    },
                    {
                        text: 'Configuring the server',
                        link: '/configuration'
                    }
                ],
                collapsed: false,
            }
        ],

        socialLinks: [
            { icon: 'github', link: 'https://github.com/SirWalross/aerionuaserver' }
        ]
    },
    srcExclude: [
        'README.md'
    ]
})
