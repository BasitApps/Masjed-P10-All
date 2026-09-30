/*
 * Created on 11.02.2004
 *
 * To change the template for this generated file go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */
package gui;

import gui.util.CharTableModel;
import gui.util.CharTablePopupMenu;

import java.awt.Dimension;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumn;

import util.Font;
import util.FontCharacter;

/**
 * @author Fabian Maximilian Thiele
 */
public class CharTable implements KeyListener, MouseListener, PropertyChangeListener {
	private DefaultTableColumnModel columnModel;
	private CharTableModel tableModel;
	private JTable charTable;
	private Font font;
	private FontEditor parent;
	private CharTablePopupMenu popup;
	
	public CharTable(Font font, FontEditor parent) {
		this.font = font;
		this.parent = parent;
		
		popup = new CharTablePopupMenu(this);
		
		columnModel = new DefaultTableColumnModel();

		TableColumn num = new TableColumn(0, 30); 
		num.setHeaderValue("Dec");
		columnModel.addColumn(num);
		TableColumn hex = new TableColumn(1, 35);
		hex.setHeaderValue("Hex"); 
		columnModel.addColumn(hex);
                TableColumn ascii = new TableColumn(2, 37);
                ascii.setHeaderValue("ASCII"); 
                columnModel.addColumn(ascii);
                TableColumn arabic = new TableColumn(3, 135);
                arabic.setHeaderValue("Arabic / Form");
                columnModel.addColumn(arabic);
                TableColumn width = new TableColumn(4, 37); 
                width.setHeaderValue("Width");
                columnModel.addColumn(width);
                TableColumn desc = new TableColumn(5, 93);
		desc.setHeaderValue("Comment"); 
		columnModel.addColumn(desc);
		
		tableModel = new CharTableModel();
                tableModel.setColumnCount(6);
		
		charTable = new JTable(tableModel, columnModel);
		charTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		charTable.setMinimumSize(new Dimension(225, 100));
		charTable.addKeyListener(this);
		charTable.addMouseListener(this);
		charTable.addPropertyChangeListener(this);
		
		doRowData();
	}
	
	private void doRowData() {
		tableModel.removeAllRows();
		
		for(int i=0, j=font.getStartIndex(); i<font.getCharCount(); i++, j++) {
			FontCharacter fc = font.getChar(i);
			
			String hexVal = (Integer.toHexString(j)).toUpperCase();
			if(hexVal.length() == 1)
				hexVal = "0"+hexVal;
			
			String asciiC = "";
			if(j >= 32 && j < 127) {
				asciiC = ""+(char)j;
			}
			
                        Object[] tableData = {
                                ""+j,
                                "0x"+hexVal,
                                asciiC,
                                arabicLabel(j),
                                ""+fc.getWidth(),
                                fc.getComment()
                        };
                        tableModel.addRow(tableData);
                }
                charTable.setRowSelectionInterval(0,0);
        }

        // BasitFont Arabic slots: 0x80..0xFE, in the same order as gen_font.py.
        private String arabicLabel(int cp) {
                final String[] letters = {
                    "\u0621", "\u0622", "\u0623", "\u0625", "\u0627",
                    "\u062F", "\u0630", "\u0631", "\u0632", "\u0648",
                    "\u0649", "\u0624", "\u0628", "\u0629", "\u062A",
                    "\u062B", "\u062C", "\u062D", "\u062E", "\u0633",
                    "\u0634", "\u0635", "\u0636", "\u0637", "\u0638",
                    "\u0639", "\u063A", "\u0641", "\u0642", "\u0643",
                    "\u0644", "\u0645", "\u0646", "\u0647", "\u0648",
                    "\u064A", "\u0626"
                };
                final int[] counts = {
                    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                    4, 4, 4, 4, 4
                };
                final String[] forms = {"ISO", "INI", "MED", "FIN"};
                int slot = 0x80;
                for (int i = 0; i < letters.length; i++) {
                        for (int f = 0; f < counts[i]; f++) {
                                if (slot == cp)
                                        return letters[i] + "  " +
                                            (counts[i] == 2 ? (f == 0 ? "ISO" : "FIN") : forms[f]);
                                slot++;
                        }
                }
                String[] lig = {"لا ISO", "لا FIN", "لأ ISO", "لأ FIN",
                                "لإ ISO", "لإ FIN", "لآ ISO", "لآ FIN"};
                if (cp >= 0xF7 && cp <= 0xFE)
                        return lig[cp - 0xF7];
                return "";
        }
        public JTable getTable() {
		return charTable;
	}
	
	public int getSelectedRow() {
		return charTable.getSelectedRow();
	}
	
	public FontCharacter getCharAt(int index) {
		FontCharacter cd = (FontCharacter) font.getChar(index);
		return cd;
	}
	
	public void setCharAt(int idx, FontCharacter fc) {
		font.setChar(fc, idx);
	}
	
	public FontCharacter getSelectedCharacter() {
		int index = charTable.getSelectedRow();
		FontCharacter fc = null;
		if(index != -1) {
			fc = (FontCharacter) font.getChar(index);
		}
		
		return fc;
	}
	
	public void setSelectedCharacter(FontCharacter fc) {
		int index = charTable.getSelectedRow();
		setCharAt(index, fc);
	}
	
	public void insertNewCharacter() {
		int index = charTable.getSelectedRow()+1;
		font.insertCharAt(index);
		doRowData();
	}
	
	public void removeSelectedCharacter() {
		int index = charTable.getSelectedRow();
		font.removeCharAt(index);
		doRowData();
	}

	/* (non-Javadoc)
	 * @see java.awt.event.KeyListener#keyTyped(java.awt.event.KeyEvent)
	 */
	public void keyTyped(KeyEvent e) {
		// unused
	}

	/* (non-Javadoc)
	 * @see java.awt.event.KeyListener#keyPressed(java.awt.event.KeyEvent)
	 */
	public void keyPressed(KeyEvent e) {
		// unused
	}

	/* (non-Javadoc)
	 * @see java.awt.event.KeyListener#keyReleased(java.awt.event.KeyEvent)
	 */
	public void keyReleased(KeyEvent e) {
		int index = charTable.getSelectedRow();
		parent.setEditingChar(index);
	}

	/* (non-Javadoc)
	 * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
	 */
	public void mouseClicked(MouseEvent arg0) {
		// unused
	}

	/* (non-Javadoc)
	 * @see java.awt.event.MouseListener#mousePressed(java.awt.event.MouseEvent)
	 */
	public void mousePressed(MouseEvent e) {
		int row = charTable.rowAtPoint(e.getPoint());
		charTable.setRowSelectionInterval(row, row);
		parent.setEditingChar(row);
		
		if(e.getButton() == MouseEvent.BUTTON3) {
			popup.show(charTable, e.getX(), e.getY());
		}
	}

	/* (non-Javadoc)
	 * @see java.awt.event.MouseListener#mouseReleased(java.awt.event.MouseEvent)
	 */
	public void mouseReleased(MouseEvent arg0) {
		// unused
	}

	/* (non-Javadoc)
	 * @see java.awt.event.MouseListener#mouseEntered(java.awt.event.MouseEvent)
	 */
	public void mouseEntered(MouseEvent arg0) {
		// unused
	}

	/* (non-Javadoc)
	 * @see java.awt.event.MouseListener#mouseExited(java.awt.event.MouseEvent)
	 */
	public void mouseExited(MouseEvent arg0) {
		// unused
	}
	
	public void propertyChange(PropertyChangeEvent e) {
		//System.out.println(e);
		int row = charTable.getSelectedRow();
		if(row != -1) {
			int col = charTable.getSelectedColumn();
                        if(col == 4) {
				try {
					int width = Integer.parseInt((String) charTable.getValueAt(row, col));
					FontCharacter fc = getSelectedCharacter();
					fc.setSize(0, 0, 0, width-fc.getWidth());
					setSelectedCharacter(fc);
					parent.editingCharSizeChanged();
				} catch(NumberFormatException ex) {
					// ignore
				}
                        } else if(col == 5) {
				String comment = (String) charTable.getValueAt(row, col);
				FontCharacter fc = getSelectedCharacter();
				fc.setComment(comment);
				setSelectedCharacter(fc);
			}
		}
	}
}
