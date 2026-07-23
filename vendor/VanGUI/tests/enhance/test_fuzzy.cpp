#include <cstdio>
// ---- verbatim copy of the shipped scorer (kept in sync) --------------------
static int FuzzySubsequenceScore(const char* hay, const char* needle)
{
    if (!needle || needle[0] == '\0') return 1;
    if (!hay) return 0;
    const char* n = needle;
    int score = 0, run = 0; bool prevSep = true;
    for (const char* h = *hay ? hay : hay; *h; ++h) {
        const char hc = (char)(*h | 0x20);
        const char nc = (char)(*n | 0x20);
        if (nc == hc) {
            score += 1;
            if (run > 0)  score += 3;
            if (prevSep)  score += 5;
            ++run; ++n;
            if (*n == '\0') return score + 1;
        } else run = 0;
        const char c = *h;
        prevSep = (c==' '||c=='-'||c==':'||c=='_'||c=='/'||c=='.');
    }
    return 0;
}
static int g_fail=0;
#define CHECK(c,m) do{ if(!(c)){printf("  FAIL: %s\n",m);++g_fail;} else printf("  ok  : %s\n",m);}while(0)
int main(){
    CHECK(FuzzySubsequenceScore("File: Open","fop")>0, "'fop' matches 'File: Open'");
    CHECK(FuzzySubsequenceScore("File: Open","xyz")==0, "'xyz' does not match");
    CHECK(FuzzySubsequenceScore("File: Open","")>0, "empty needle matches");
    // word-boundary + contiguous 'open' should outrank scattered 'fie'
    int sOpen = FuzzySubsequenceScore("File: Open","open");
    int sScattered = FuzzySubsequenceScore("File: Open","fie");
    CHECK(sOpen>sScattered, "contiguous word-start 'open' outranks scattered 'fie'");
    // exact prefix contiguous beats subsequence
    int sPref = FuzzySubsequenceScore("Open File","open");
    int sSub  = FuzzySubsequenceScore("Other Pane","open");
    CHECK(sPref>sSub, "prefix-contiguous beats spread subsequence");
    printf("\n%s (%d failure%s)\n", g_fail?"TESTS FAILED":"ALL TESTS PASSED", g_fail, g_fail==1?"":"s");
    return g_fail?1:0;
}
