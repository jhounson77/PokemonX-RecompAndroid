# Pokémon X Recomp Android

**V0.1.1 Beta** — projeto experimental de recompilação estática de Pokémon X para Android ARM64, com uma interface adaptada para jogar em uma ou duas telas.

A CPU do jogo usa código nativo gerado pelo **Hexlith**. **Azahar** fornece kernel, serviços do 3DS, memória e auxilia na tradução gráfica para Vulkan. Instruções e módulos que não possuem uma tradução nativa válida usam o fallback do Hexlith.

## Instalação e uso

1. Instale o APK da seção **Releases**. A versão Beta usa o mesmo pacote e a mesma assinatura das builds de teste anteriores.
2. Em **Configurações → Jogo e arquivos**, importe sua própria cópia compatível do jogo. O arquivo original do jogo não faz parte deste repositório.
3. Ajuste resolução, driver Vulkan e controles antes de iniciar.
4. Salve pelo menu do próprio jogo. Feche a sessão antes de importar ou exportar saves.

A versão pública `0.1.1-beta` corrige o retorno da tela após fechar a bolsa ou a seleção de Pokémon com módulos auxiliares ainda carregados. O `versionCode` é **35**, para permitir atualização sobre os APKs de teste sem precisar desinstalar o aplicativo.

## Funcionalidades

- Launcher e configurações nativos para Android, com tema minimalista.
- Interface do aplicativo em Português (Brasil), Inglês, Espanhol e Francês. O idioma do jogo é independente.
- Modos de uma tela e duas telas.
- Interface de batalha integrada à tela principal, controles touch e suporte a gamepad.
- Bolsa e equipe em tela secundária ampliada, com retorno à principal quando os menus auxiliares são encerrados.
- Menu original acessível pelo botão X em um painel no canto inferior direito.
- Resolução **interna** de 1× (400×240), 2× (800×480), 3× (1200×720), 4× (1600×960) e automática até 4×. Não há modo de renderização exato em 1080p.
- Seleção/importação de drivers Vulkan compatíveis.
- Áudio estéreo por AAudio e DSP do Azahar.
- FPS, velocidade de simulação, RAM do aplicativo e temperatura da bateria, com opção de ocultar os indicadores.
- Logs separados por sessão e exportação de diagnóstico.
- Instalação opcional da atualização **1.5 compatível**, em CIA ou RAR, com ativação/desativação reversível e bibliotecas nativas próprias para essa versão.
- Instalação de **packs de texturas PNG em ZIP no formato Citra/Azahar**, com opção de ativar/desativar.
- Exportação de save em ZIP e importação de ZIP ou arquivo `main`, com backup automático do save anterior.

## Desempenho

O desenvolvimento prioriza remover trabalho repetido sem alterar o tempo de simulação ou omitir efeitos do jogo. Esta Beta inclui cache persistente de shaders conhecidos, preparação dos shaders disponíveis antes da sessão, sondagem assíncrona da interface e reutilização de buffers de controles.

Com o 3D estereoscópico desligado, a apresentação reutiliza a imagem esquerda no descritor do olho direito, evitando a preparação duplicada. Em testes de código de produção com dispositivos simulados, a preparação passou de três para duas imagens e 10.000 consultas de geometria da interface passaram de 60.000 alocações temporárias de C++ para zero. **Esses números medem trabalho evitado no host, não um ganho de FPS medido no celular.**

A primeira execução de efeitos ainda não vistos pode compilar novos shaders. Importar o CIA não permite conhecer antecipadamente todos os shaders que o jogo irá utilizar. Resoluções e packs de texturas maiores também aumentam o custo de GPU e memória. Não há promessa de 100% de velocidade em todas as cenas, aparelhos ou configurações.

## Limitações da Beta

- A integração em uma tela usa sinais do carregamento dos módulos e leitura visual reduzida da segunda tela; algumas transições ainda podem precisar de ajustes.
- Nomes e tipos dos golpes vêm da renderização atual do próprio jogo. A leitura estrutural de IDs/PP na versão 1.5 é experimental e só fornece PP quando há um único Pokémon identificável com segurança; os demais casos mantêm os campos desenhados pelo jogo.
- Mods de RomFS, patches de executável, IPS/BPS, cheats, plugins e savestates não são suportados pela interface atual.
- O suporte ao update é limitado ao executável verificado; outra região, revisão ou modificação pode ser recusada. Não reutilizamos os endereços da versão base para executar a atualização.
- O áudio pode exigir reinício da sessão após desconexão/troca de rota. Recuperação automática completa e gerenciamento de foco de áudio ainda não foram implementados.
- Os testes locais de regressão não substituem testes de gameplay, drivers, temperatura e consumo no aparelho.

## Desenvolvimento

O projeto evoluiu por builds de teste iterativas, com relatos de gameplay e ajustes de controles, apresentação, memória, áudio, shaders e interface. As bases técnicas e dependências conservam seus créditos e licenças originais.

- [Arquitetura e proveniência](docs/ARCHITECTURE.md)
- [Compilação e insumos externos](docs/BUILD.md)
- [Histórico técnico](docs/ANDROID_DEVELOPMENT_HISTORY.md)
- [Notas da V0.1.1 Beta](docs/RELEASE_v0.1.1-beta.md)
- [Notas da V0.1.0 Beta](docs/RELEASE_v0.1.0-beta.md)
- [Créditos e licenças](THIRD_PARTY_NOTICES.md)

## Contribuições e relatos

Ao reportar um problema, informe versão do aplicativo, aparelho, versão do Android, driver, resolução, presença do update e pack de texturas, e passos para reproduzir. Anexe um log revisado por você, sem dados pessoais, saves, arquivos do jogo ou chaves.

Não envie ROMs, CIA/3DS/CXI, código gerado a partir do jogo, saves, chaves, keystores ou senhas em commits, issues ou pull requests.

## English summary

Experimental Android ARM64 static recompilation of Pokémon X. Hexlith supplies native CPU code with interpreter fallback; Azahar supplies the 3DS platform, Vulkan graphics and DSP. Includes single/dual-screen modes, touch/gamepad controls, four app languages, compatible update 1.5 installation, PNG texture packs and save backup/import/export. This is a Beta: sustained device performance and full compatibility are not guaranteed. Game dumps, generated game sources and private build/signing inputs are excluded from this source repository. See the linked build and architecture documentation for the external inputs required to reproduce a binary.
