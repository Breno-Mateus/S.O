#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

unsigned long long funcao_s(unsigned long long n) {
    unsigned long long contador = 0;
    while (n != 1) {
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = (3 * n) + 1;
        }
        contador++;
    }
    return contador;
}

unsigned long long executa_trabalho_bloco(unsigned long long start, unsigned long long end) {
    if (start > end) return 0;
    unsigned long long soma_passos = 0;
    for (unsigned long long i = start; i <= end; i++) {
        soma_passos += funcao_s(i);
    }
    return soma_passos;
}

unsigned long long executa_trabalho_ciclico(unsigned long long A, unsigned long long B, unsigned long long W, unsigned long long w) {
    unsigned long long soma_passos = 0;
    for (unsigned long long i = A + w; i <= B; i += W) {
        soma_passos += funcao_s(i);
    }
    return soma_passos;
}

void funcao_thread(unsigned long long A, unsigned long long B, unsigned long long W, unsigned long long w, const std::string& particao, unsigned long long L, double& tempo_saida, unsigned long long& soma_saida) {
    auto inicio = std::chrono::high_resolution_clock::now();
    unsigned long long soma_passos = 0;

    if (particao == "bloco") {
        unsigned long long base_bloco = L / W;
        unsigned long long resto = L % W;
        unsigned long long tamanho_atual = base_bloco + (w < resto ? 1 : 0);
        unsigned long long start = A + (w * base_bloco) + std::min(w, resto);
        unsigned long long end = start + tamanho_atual - 1;

        soma_passos = executa_trabalho_bloco(start, end);
    } else if (particao == "ciclico") {
        soma_passos = executa_trabalho_ciclico(A, B, W, w);
    }

    auto fim = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duracao = fim - inicio;
    tempo_saida = duracao.count();
    soma_saida = soma_passos;
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        std::cerr << "Uso: " << argv[0] << " <A> <B> <W> <modo> <particao> <arquivo_saida>\n";
        return 1;
    }

    unsigned long long A = std::stoull(argv[1]);
    unsigned long long B = std::stoull(argv[2]);
    unsigned long long W = std::stoull(argv[3]);
    std::string modo = argv[4];
    std::string particao = argv[5];
    std::string arquivo_saida = argv[6];

    unsigned long long L = (B - A) + 1;

    auto inicio_total = std::chrono::high_resolution_clock::now();

    double tempo_min_filho = -1.0;
    double tempo_max_filho = -1.0;
    double tempo_agregacao = -1.0;
    unsigned long long soma_total = 0;

    if (W == 1) {
        if (particao == "bloco") {
            soma_total = executa_trabalho_bloco(A, B);
        } else {
            soma_total = executa_trabalho_ciclico(A, B, 1, 0);
        }
        std::cout << "[Sequencial W=1] Total de passos acumulados: " << soma_total << std::endl;
    }
    else if (modo == "processo") {
        unsigned long long base_bloco = L / W;
        unsigned long long resto = L % W;

        for (unsigned long long w = 0; w < W; w++) {
            pid_t pid = fork();

            if (pid < 0) {
                std::cerr << "Erro no fork\n";
                return 1;
            }
            else if (pid == 0) {
                auto inicio_filho = std::chrono::high_resolution_clock::now();
                unsigned long long soma_passos = 0;

                if (particao == "bloco") {
                    unsigned long long tamanho_atual = base_bloco + (w < resto ? 1 : 0);
                    unsigned long long start = A + (w * base_bloco) + std::min(w, resto);
                    unsigned long long end = start + tamanho_atual - 1;

                    soma_passos = executa_trabalho_bloco(start, end);
                } else if (particao == "ciclico") {
                    soma_passos = executa_trabalho_ciclico(A, B, W, w);
                }

                auto fim_filho = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duracao = fim_filho - inicio_filho;

                std::cout << "[Processo Filho " << w << "] Passos calculados: " << soma_passos << std::endl;

                std::ofstream arq("parcial_" + std::to_string(w) + ".txt");
                arq << duracao.count() << " " << soma_passos;
                arq.close();
                exit(0);
            }
        }

        for (unsigned long long w = 0; w < W; w++) {
            wait(NULL);
        }

        auto inicio_agreg = std::chrono::high_resolution_clock::now();

        for (unsigned long long w = 0; w < W; w++) {
            std::string nome_arq = "parcial_" + std::to_string(w) + ".txt";
            std::ifstream arq(nome_arq);
            double t_filho;
            unsigned long long s_filho;

            if (arq >> t_filho >> s_filho) {
                soma_total += s_filho;
                if (w == 0) {
                    tempo_min_filho = t_filho;
                    tempo_max_filho = t_filho;
                } else {
                    if (t_filho < tempo_min_filho) tempo_min_filho = t_filho;
                    if (t_filho > tempo_max_filho) tempo_max_filho = t_filho;
                }
            }
            arq.close();
            std::remove(nome_arq.c_str());
        }

        auto fim_agreg = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dur_agreg = fim_agreg - inicio_agreg;
        tempo_agregacao = dur_agreg.count();

        std::cout << "[Pai - Processos] Total de passos agregados: " << soma_total << std::endl;
    }
    else if (modo == "thread") {
        std::vector<std::thread> threads;
        std::vector<double> tempos_threads(W, 0.0);
        std::vector<unsigned long long> somas_threads(W, 0);

        for (unsigned long long w = 0; w < W; w++) {
            threads.push_back(std::thread(funcao_thread, A, B, W, w, std::ref(particao), L, std::ref(tempos_threads[w]), std::ref(somas_threads[w])));
        }

        for (unsigned long long w = 0; w < W; w++) {
            threads[w].join();
        }

        auto inicio_agreg = std::chrono::high_resolution_clock::now();

        tempo_min_filho = tempos_threads[0];
        tempo_max_filho = tempos_threads[0];

        for (unsigned long long w = 0; w < W; w++) {
            soma_total += somas_threads[w];
            std::cout << "[Thread " << w << "] Passos calculados: " << somas_threads[w] << std::endl;
            if (tempos_threads[w] < tempo_min_filho) tempo_min_filho = tempos_threads[w];
            if (tempos_threads[w] > tempo_max_filho) tempo_max_filho = tempos_threads[w];
        }

        auto fim_agreg = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> dur_agreg = fim_agreg - inicio_agreg;
        tempo_agregacao = dur_agreg.count();

        std::cout << "[Pai - Threads] Total de passos agregados: " << soma_total << std::endl;
    }

    auto fim_total = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tempo_total = fim_total - inicio_total;

    std::ofstream out(arquivo_saida, std::ios::app);
    out << std::scientific << std::setprecision(2);
    out << modo << "," << particao << "," << W << "," << L << "," << tempo_total.count() << ",";

    if (W == 1) {
        out << "-1,-1,-1\n";
    } else {
        out << tempo_max_filho << "," << tempo_min_filho << "," << tempo_agregacao << "\n";
    }

    out.close();

    return 0;
}

// matricula 20250028277
// M = 28277