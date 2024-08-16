/*ident "@(#)ccsdemos:sc_demos/Map_demo.C	1.1" */

#include <String.h>
#include <Map.h>
#include <iostream.h>

int main() {
        Map<String,int> count;
        String word;

        while (cin >> word)
                count[word]++;

	Mapiter<String,int> p(count);
	while (p.next())
		cout << p.curr()->value << '\t' << p.curr()->key << endl;

	return 0;
}
