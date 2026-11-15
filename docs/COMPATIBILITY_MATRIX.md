# Matriz de compatibilidade

**GO** para comparação de código; **BLOCKED** para concluir compatibilidade do jogo.

Comparação executada: `git -C vendor/runtime diff --stat c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7 99078b119b5d5e0d1d6afb1390e29437c0c984c0`. [Resultado bruto](evidence/runtime-diff.txt).

| Caminho/contrato | Comparação | Decisão Android |
|---|---|---|
| `runtime/cpu/x86_context.h`, `x86_ops.h`, `dispatch.cpp` | Idênticos | Base Burner; igualdade de fonte não comprova código gerado ARM64 nem ABI do título. |
| `runtime/kernel/memory.h`, `memory.cpp`, `kernel.h`, threads/arquivos | Idênticos entre pins | Portabilidade Android ainda precisa de implementação e teste; igualdade não elimina APIs Windows. |
| `runtime/xdk/hle.h`, `layout.h` | Idênticos | Preservar contrato e verificar consumidores Urban Chaos em G1. |
| `runtime/kernel/xboxkrnl.cpp` | Diferente: flags estéreo nas consultas EEPROM | Corrige a generalização inicial sobre o kernel. Manter pin Burner; qualquer backport exige evidência e patch explícito. |
| `runtime/audio/xaudio2.cpp` | Diferente: fila adaptativa após starvation | XAudio2 não fornece backend Android. Áudio SDL3 do jogo segue bloqueado. |
| `runtime/xdk/dsound.cpp` | Diferente | Não transplantado. Testar comportamento original de áudio em G3. |
| `runtime/gpu/d3d12.cpp` | Diferente | D3D12 não é backend Android; não faz parte do diagnóstico. |
| `runtime/gpu/shaders.cpp`, `shaders.h` | Diferentes | Tradução D3D8→Vulkan do título ainda não implementada/validada. |
| `README.md`, `templates/game/README.md`, `templates/game/release.bat` | Diferentes | Documentação/empacotamento; não determinam compatibilidade ABI. |
| Adaptações Black Android | MIT no projeto, contratos próprios de Black | Referência inspecionada, sem copiar endereços, hooks, skins ou estruturas. |
| Adaptações Burner | Licença geral não esclarecida | **BLOCKED** para reutilização dessas adaptações. |
| Diagnóstico novo SDL3/Vulkan | Compilado sem código de jogo | **GO** para distribuir debug e testar manualmente; não estabelece G2–G3. |

São nove caminhos modificados no total. O runtime pinado pela Burner permanece sem alterações locais. Não há compatibilidade declarada com o hash candidato até G1 e os testes dependentes.
