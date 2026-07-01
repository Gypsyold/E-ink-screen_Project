const app = getApp()

Page({
  data: {
    books: [],
    loading: false,
    syncedAt: '',
    notSynced: false
  },

  onShow() {
    this.loadBooks()
  },

  onPullDownRefresh() {
    this.loadBooks(() => wx.stopPullDownRefresh())
  },

  loadBooks(callback) {
    this.setData({ loading: true })
    wx.request({
      url: `${app.globalData.apiBase}/device.php?action=list`,
      success: (res) => {
        if (res.data && res.data.status === 'ok') {
          const source    = res.data.source || ''
          const notSynced = source === 'not_synced'
          const books     = (res.data.books || []).map((b, idx) => ({
            ...b,
            idx:   idx + 1,
            title: b.title || gbkHexToDisplay(b.fn_gbk_hex) || `文件${idx + 1}`,
          }))
          this.setData({
            books,
            notSynced,
            syncedAt: res.data.synced_at || ''
          })
        } else {
          wx.showToast({ title: '加载失败', icon: 'error' })
        }
      },
      fail: () => wx.showToast({ title: '网络错误', icon: 'error' }),
      complete: () => {
        this.setData({ loading: false })
        callback && callback()
      }
    })
  },

  deleteBook(e) {
    const item = e.currentTarget.dataset.item
    wx.showModal({
      title: '删除书籍',
      content: `从墨水屏中删除《${item.title}》？此操作无法撤销。`,
      confirmText: '删除',
      confirmColor: '#e53935',
      success: (res) => {
        if (!res.confirm) return
        wx.request({
          url: `${app.globalData.apiBase}/device.php?action=add_delete&fn_gbk_hex=${item.fn_gbk_hex}`,
          success: (r) => {
            if (r.data && r.data.status === 'ok') {
              wx.showToast({ title: '删除请求已提交', icon: 'success' })
              this.loadBooks()
            } else {
              wx.showToast({ title: r.data.msg || '操作失败', icon: 'error' })
            }
          },
          fail: () => wx.showToast({ title: '网络错误', icon: 'error' })
        })
      }
    })
  }
})

/* 将 GBK hex 转为可显示文字（仅 ASCII 部分可解，中文显示 hex 后几位作提示）*/
function gbkHexToDisplay(hex) {
  if (!hex) return ''
  try {
    let s = ''
    for (let i = 0; i < hex.length - 1; i += 2) {
      const b = parseInt(hex.substr(i, 2), 16)
      if (b < 0x80) s += String.fromCharCode(b)
    }
    /* 若全为不可显示字节，直接截取 hex 末尾作区分 */
    return s.replace(/\.txt$/i, '') || hex.slice(-8)
  } catch (e) {
    return hex.slice(-8)
  }
}
