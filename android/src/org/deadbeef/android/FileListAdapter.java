package org.deadbeef.android;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class FileListAdapter extends ArrayAdapter<String> {
    private List<String> items;
    private Context myContext;

    public FileListAdapter(Context context, int resource, int textViewResourceId, List<String> items) {
        super(context, resource, textViewResourceId, items);
        this.items = items;
        this.myContext = context;
    }

    @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View v = convertView;
            if (v == null) {
                LayoutInflater vi = (LayoutInflater)myContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                v = vi.inflate(R.layout.plitem, null);
            }
            String o = items.get(position);

            if (o != null) {                                
                TextView tt = (TextView) v.findViewById(R.id.title);
                if (tt != null) {
                    tt.setText(o);
                }
            }

            return convertView;
        }
}

