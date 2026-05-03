#include <iostream>
#include <string>
#include <limits>
#include <cstring>
#include "heapFile.h"
#include "pageHeader.h"
#include "slot.h"

using namespace std;

static Record makeRecord(int32_t id, const string& name) {
    Record r;
    int32_t nameLen = static_cast<int32_t>(name.size());
    r.data.resize(sizeof(int32_t) + sizeof(int32_t) + name.size());
    char* p = r.data.data();
    memcpy(p, &id, sizeof(int32_t));
    memcpy(p + sizeof(int32_t), &nameLen, sizeof(int32_t));
    memcpy(p + 2 * sizeof(int32_t), name.data(), name.size());
    return r;
}

static void printRecord(const Record& r) {
    if (r.size() < 8) {
        cout << "<corrupt record>\n";
        return;
    }
    int32_t id;
    int32_t nameLen;
    memcpy(&id, r.data.data(), sizeof(int32_t));
    memcpy(&nameLen, r.data.data() + sizeof(int32_t), sizeof(int32_t));
    string name(r.data.data() + 2 * sizeof(int32_t), r.data.data() + 2 * sizeof(int32_t) + nameLen);
    cout << "id=" << id << " name='" << name << "'\n";
}

int main() {
    string path = "heapfile.dat";
    HeapFile heap(path);

    while (true) {
        cout << "\nMenu:\n";
        cout << "1) Insert user\n";
        cout << "2) Read by RID\n";
        cout << "3) Update by RID\n";
        cout << "4) Delete logical\n";
        cout << "5) Delete physical\n";
        cout << "6) List pages/slots\n";
        cout << "0) Exit\n";
        cout << "> ";
        int opt;
        if (!(cin >> opt)) break;
        if (opt == 0) break;

        if (opt == 1) {
            int32_t id;
            string name;
            cout << "id: "; cin >> id;
            cout << "name: "; cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, name);
            Record r = makeRecord(id, name);
            RID rid = heap.insert(r);
            cout << "Inserted at (" << rid.pageId << "," << (int)rid.slotId << ")\n";
        } else if (opt == 2) {
            uint32_t pid; int sid;
            cout << "pageId: "; cin >> pid; cout << "slotId: "; cin >> sid;
            auto rec = heap.read(RID{pid, static_cast<uint8_t>(sid)});
            if (rec) printRecord(*rec);
            else cout << "Not found or invalid slot\n";
        } else if (opt == 3) {
            uint32_t pid; int sid; string name;
            cout << "pageId: "; cin >> pid; cout << "slotId: "; cin >> sid;
            cout << "new name: "; cin.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(cin, name);
            // read existing to get id
            auto old = heap.read(RID{pid, static_cast<uint8_t>(sid)});
            if (!old) { cout << "Record not found\n"; continue; }
            int32_t id;
            memcpy(&id, old->data.data(), sizeof(int32_t));
            Record nr = makeRecord(id, name);
            RID newRid = heap.update(RID{pid, static_cast<uint8_t>(sid)}, nr);
            cout << "Updated. New location: (" << newRid.pageId << "," << (int)newRid.slotId << ")\n";
        } else if (opt == 4) {
            uint32_t pid; int sid;
            cout << "pageId: "; cin >> pid; cout << "slotId: "; cin >> sid;
            bool ok = heap.deleteLogical(RID{pid, static_cast<uint8_t>(sid)});
            cout << (ok ? "Deleted logically\n" : "Delete failed\n");
        } else if (opt == 5) {
            uint32_t pid; int sid;
            cout << "pageId: "; cin >> pid; cout << "slotId: "; cin >> sid;
            bool ok = heap.deletePhysical(RID{pid, static_cast<uint8_t>(sid)});
            cout << (ok ? "Deleted physically\n" : "Delete failed\n");
        } else if (opt == 6) {
            uint32_t pc = heap.pageCount();
            cout << "Pages: " << pc << "\n";
            for (uint32_t i = 0; i < pc; ++i) {
                Page p = heap.readPage(i);
                PageHeader* h = reinterpret_cast<PageHeader*>(p.raw());
                cout << "Page " << i << ": slots=" << (int)h->slotCount << " deleted=" << (int)h->deletedSlotCount << " freeOffset=" << h->freeSpaceOffset << " slotDirOffset=" << h->slotDirectoryOffset << " freeSpace=" << h->freeSpace << "\n";
                for (uint8_t s = 0; s < h->slotCount; ++s) {
                    size_t slotOff = Page::PAGE_SIZE - sizeof(Slot) * (static_cast<size_t>(s) + 1);
                    const Slot* sp = reinterpret_cast<const Slot*>(p.raw() + slotOff);
                    cout << "  slot " << (int)s << ": off=" << sp->offset << " len=" << sp->length << " flags=" << (int)sp->flags << "\n";
                }
            }
        }
    }

    return 0;
}

