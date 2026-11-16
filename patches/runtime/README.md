# Experimentos de integração do SDK

`directsound-device-lifetime.patch` aplica-se exclusivamente ao XboxXBERecomp no pin `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`. Deriva do runtime MIT; aviso em [XboxXBERecomp-MIT.txt](../../licenses/XboxXBERecomp-MIT.txt). Não contém implementação Burner, código guest ou dados comerciais.

O patch implementa contagem de referências do dispositivo DirectSound dentro do modelo HLE existente: criação adquire referência, `AddRef` incrementa, `Release` decrementa e libera o objeto ao chegar a zero. Acesso a objeto desconhecido interrompe o experimento em vez de retornar sucesso fictício. O mixer original do SDK permanece o mesmo.

**Estado: experimental, validado apenas no host Linux de pesquisa.** Não está aplicado ao checkout pinado nem aos builds padrão. Não é implementação completa de DirectSound ou correção geral de áudio.

A nova função requer `SYM_IDirectSound_AddRef` com endereço e ABI revisados para o XBE usado. Não adivinhar endereço nem reutilizar o de Black. O novo símbolo deve constar no header de metadados ou em uma definição explícita do build privado. Aplicar somente a uma cópia ignorada do SDK, por exemplo com `git apply --directory=<cópia-privada>` a partir da raiz apropriada, e incluir essa cópia de `dsound.cpp` no lugar da original. Não compilar ambas.

O teste [test_device_lifetime.cpp](../../host/audio-device/test_device_lifetime.cpp) foi compilado no host de pesquisa com o mesmo header de símbolos, pin, patch e flags do runtime. `AudioSelfTest()` é chamado antes do boot em um processo novo; o processo encerra com seu resultado. Ele usa imagem vazia e memória sintética, executa oito chamadas HLE reais e verifica retorno, limpeza da pilha stdcall, propriedade compartilhada entre criações, liberação final e recriação. Não executa instruções recompiladas do jogo. Não é um job CI público nem um teste de saída de som.

Ver [investigação e testes progressivos](../../docs/AUDIO_INVESTIGATION.md) para as falhas posteriores. A integração desse patch ao host de produção e os testes de streams/buffers/callbacks continuam pendentes.
