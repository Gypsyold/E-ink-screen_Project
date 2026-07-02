const app = getApp()

Page({
  data: {
    books: [],
    loading: false,
    uploading: false
  },

  onShow() {
    this.loadBooks()
  },

  onPullDownRefresh() {
    this.loadBooks(() => wx.stopPullDownRefresh())
  },

  loadBooks(callback) {
    this.setData({ loading: true })
    const url = `${app.globalData.apiBase}/books.php`
    console.log('[loadBooks] 请求:', url)
    wx.request({
      url,
      timeout: 15000,
      success: (res) => {
        console.log('[loadBooks] 响应 statusCode:', res.statusCode, 'data:', JSON.stringify(res.data))
        if (res.data && res.data.status === 'ok') {
          const self = this
          const books = (res.data.books || []).reverse().map(function(b) {
            return Object.assign({}, b, {
              title: b.title || b.filename || '未知书名',
              sizeText: self.formatSize(b.size_bytes),
              dateText: (b.uploaded_at || '').substring(0, 10)
            })
          })
          console.log('[setData] books:', JSON.stringify(books))
          this.setData({ books: books })
        } else {
          wx.showToast({ title: '加载失败', icon: 'error' })
        }
      },
      fail: (err) => {
        console.error('[loadBooks] 失败:', JSON.stringify(err))
        wx.showToast({ title: '网络错误: ' + (err.errMsg || ''), icon: 'none', duration: 3000 })
      },
      complete: () => {
        this.setData({ loading: false })
        callback && callback()
      }
    })
  },

  uploadBook() {
    wx.chooseMessageFile({
      count: 1,
      type: 'file',
      extension: ['txt'],
      success: (res) => {
        const file = res.tempFiles[0]
        this.setData({ uploading: true })

        wx.uploadFile({
          url: `${app.globalData.apiBase}/upload.php`,
          filePath: file.path,
          name: 'file',
          formData: { filename: file.name },
          success: (uploadRes) => {
            try {
              const data = JSON.parse(uploadRes.data)
              if (data.status === 'ok') {
                wx.showToast({ title: '上传成功', icon: 'success' })
              } else {
                wx.showToast({ title: data.msg || '上传失败', icon: 'error' })
              }
            } catch (e) {
              wx.showToast({ title: '服务器响应异常', icon: 'error' })
            }
          },
          fail: () => {
            wx.showToast({ title: '上传失败，请检查网络', icon: 'error' })
          },
          complete: () => {
            this.setData({ uploading: false })
            this.loadBooks()
          }
        })
      }
    })
  },

  addToQueue(e) {
    const bookId = e.currentTarget.dataset.id
    wx.showLoading({ title: '加入队列中...' })

    wx.request({
      url: `${app.globalData.apiBase}/queue.php`,
      method: 'POST',
      header: { 'content-type': 'application/json' },
      data: { book_id: bookId },
      success: (res) => {
        if (res.data && res.data.status === 'ok') {
          wx.showToast({ title: '已加入队列', icon: 'success' })
        } else {
          wx.showToast({ title: res.data.msg || '操作失败', icon: 'error' })
        }
      },
      fail: () => {
        wx.showToast({ title: '网络错误', icon: 'error' })
      },
      complete: () => {
        wx.hideLoading()
      }
    })
  },

  deleteBook(e) {
    const bookId = e.currentTarget.dataset.id
    wx.showModal({
      title: '确认删除',
      content: '删除后书籍文件将从服务器移除',
      success: (res) => {
        if (!res.confirm) return
        wx.request({
          url: `${app.globalData.apiBase}/books.php?action=delete&id=${bookId}`,
          method: 'POST',
          success: () => {
            wx.showToast({ title: '已删除', icon: 'success' })
            this.loadBooks()
          }
        })
      }
    })
  },

  formatSize(bytes) {
    if (!bytes) return '未知'
    if (bytes < 1024) return bytes + ' B'
    if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB'
    return (bytes / 1048576).toFixed(1) + ' MB'
  }
})
