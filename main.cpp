// -*- compile-command: "make main"; -*-
// #include "header.h" -- precompiled
#include "instance.h"
#include "sa.h"
#include "tour.h"

// initial tour : 2-approximation of the tsp problem, O(n^2)
vi initial_tour(instance const& I) {
  cerr << "Computing initial tour..." << endl;
  double t0 = timeInfo.getTime();

  int n = I.n;
  vector<bool>   E(n); // explored
  vector<double> D2(n,1e100); // min distance squared
  vi             P(n,-1); // parent
  vvi            G(n); // mst
  int next=0;
  auto add = [&](int i) {
    E[i]=1; next=-1; double minD=1e100;
    FOR(j,n) if(!E[j]) {
      double d2 = I.dist2(i,j);
      if(d2 < D2[j]) {
        D2[j] = d2;
        P[j] = i;
      }
      if(D2[j]<minD) { minD = D2[j]; next=j; }
    }
  };
  FOR(i,n) {
    if(!(i&63)) cout << i << "/" << n << flush << P_BEGIN;
    if(P[next] != -1) G[P[next]].pb(next);
    add(next);
  }

  vi tour; tour.reserve(n-1);
  function<void(int)> dfs = [&](int i) {
    if(i) tour.pb(i);
    for(int j : G[i]) dfs(j);
  };
  dfs(0);
  assert((int)tour.size() == n-1);

  cout << "Done (" << timeInfo.getTime()-t0 << "s)" << endl;
  return tour;
}

double tour_length(instance const& I, vi const& tour) {
  int n = I.n;
  assert((int)tour.size() == n-1);
  double len = 0.0;
  int last=0;
  FOR(i,n-1) { len += I.dist(last,tour[i]); last=tour[i]; }
  len += I.dist(last,0);
  return len;
}

array_tour tsp(instance const& I, vi const& tour) {
  int n = I.n;
  array_tour S(tour);
  double score = tour_length(I,tour);
  vector<sa_move<array_tour>> moves;
  // 2-opt
  struct {
    int i,j;
  } move2opt;
  moves.pb(sa_move<array_tour>
           { 1,
               ([&]() {
                 do {
                   move2opt.i=random(n-1);
                   move2opt.j=random(n-1);
                 } while(move2opt.i==move2opt.j);
                 if(move2opt.i>move2opt.j) swap(move2opt.i,move2opt.j);
               }),
               ([&](array_tour const& ar) -> double {
                 int i=move2opt.i;
                 int j=move2opt.j;
                 double delta = 0;
                 if(i == 0) {
                   delta += I.dist(0, ar.tour[i]);
                   delta -= I.dist(0, ar.tour[j]);
                 } else {
                   delta += I.dist(ar.tour[i-1], ar.tour[i]);
                   delta -= I.dist(ar.tour[i-1], ar.tour[j]);
                 }
                 if(j == n-2) {
                   delta += I.dist(0, ar.tour[j]);
                   delta -= I.dist(0, ar.tour[i]);
                 } else {
                   delta += I.dist(ar.tour[j], ar.tour[j+1]);
                   delta -= I.dist(ar.tour[i], ar.tour[j+1]);
                 }
                 return -delta;
               }),
               ([&](array_tour& ar) {
                 int i=move2opt.i;
                 int j=move2opt.j;
                 reverse(ar.tour.data()+i, ar.tour.data()+j+1);
               })
               });
  simulated_annealing<array_tour>
    (S, score,
     30.0, sqrt(score), // other max temperature ?
     (1<<12), (1<<22),
     moves,
     [&](double t, double sc, array_tour const& S_) {
      if(t>0.9) {
        score = sc;
        S = S_;
      }
     });
  return S;
}

void greedy_packing(instance const& I, vi const& tour) {
  int n = I.n, m = I.m;
  // solution S;
  // if(initialS) S.from_partial(*initialS);
  // double curScore = S.score();

  // vector<double> D(n);
  // D[0] = 0; D[S.A.back()] = I.dist(0,S.A.back());
  // FORD(i,n-3,0) D[S.A[i]] = D[S.A[i+1]] + I.dist(S.A[i],S.A[i+1]);
  // vi J(m); iota(all(J), 0);
  // auto itemValue = [&](int i) -> double {
  //   return (double) I.items[i].p / ((double) I.items[i].w * (double) D[I.items[i].node]);
  // };
  // sort(all(J), [&](int i, int j) {
  //     return itemValue(i) > itemValue(j);
  //   });

  // auto add = [&]() -> bool {
  //   if(S.TW > I.W) return 0;
  //   double score = S.score();
  //   if(score >= curScore) {
  //     curScore = score;
  //     return 1;
  //   }else{
  //     return 0;
  //   }
  // };

  // FOR(k,16) for(int i : J) { S.flipB(i); if(!add()) S.flipB(i); }
  // return S;
}


int main(int argc, char** argv){
  random_reset(time(0));

  assert(argc >= 2);
  char *filename = argv[1];

  double time_0 = timeInfo.getTime();

  instance I;

  { ifstream F(filename);
    I.read(F);
    if(!I.validateAndInit()) {
      throw runtime_error("Bad instance file...");
    }
  }
  I.print();

  vi tour = initial_tour(I);
  cout << "Length: " << tour_length(I, tour) << endl;

  array_tour tour2 = tsp(I,tour);
  greedy_packing(I,tour2.tour);

  cerr << "Elapsed: " << (timeInfo.getTime()-time_0) << "s" << endl;

  return 0;
}
