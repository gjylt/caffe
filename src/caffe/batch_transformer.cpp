#include "caffe/batch_transformer.hpp"

namespace caffe {

template<typename Ftype, typename Btype>
BatchTransformer<Ftype, Btype>::BatchTransformer(int target_device, size_t rank_,
    size_t queues_num, const TransformationParameter& transform_param) :
    InternalThread(target_device, rank_, 1, false),
      queues_num_(queues_num), next_batch_queue_(0UL), transform_param_(transform_param) {
//  boost::shared_ptr<TBlob<Btype>> processed = make_shared<TBlob<Btype>>();
  shared_ptr<Batch> processed = make_shared<Batch>(tp<Ftype>(), tp<Ftype>());
  processed_free_.push(processed);
  prefetches_free_.resize(queues_num_);
  prefetches_full_.resize(queues_num_);
  for (size_t i = 0; i < queues_num_; ++i) {
    shared_ptr<Batch> batch = make_shared<Batch>(tp<Ftype>(), tp<Ftype>());
    prefetch_.push_back(batch);
    prefetches_free_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
    prefetches_full_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
    prefetches_free_[i]->push(batch);
  }
  tmp_.safe_reshape_mode(true);
  StartInternalThread();
}

template<typename Ftype, typename Btype>
void BatchTransformer<Ftype, Btype>::ResizeQueues(size_t queues_num) {
  StopInternalThread();
  queues_num_ = queues_num;
  size_t size = prefetches_free_.size();
  if (queues_num_ > size) {
    prefetches_free_.resize(queues_num_);
    prefetches_full_.resize(queues_num_);
    for (size_t i = size; i < queues_num_; ++i) {
      shared_ptr<Batch> batch = make_shared<Batch>(tp<Ftype>(), tp<Ftype>());
      prefetch_.push_back(batch);
      prefetches_free_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
      prefetches_full_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
      prefetches_free_[i]->push(batch);
    }
    next_batch_queue();  // 0th already processed
  }
  StartInternalThread();
}

template<typename Ftype, typename Btype>
void BatchTransformer<Ftype, Btype>::InternalThreadEntry() {
//  shared_ptr<Batch> batch =
//      prefetches_full_[next_batch_queue_]->pop("Data layer prefetch queue empty");

//  if (batch->data_packing() == this->transform_param_.forward_packing()
//      && top[0]->shape() == batch->data_->shape()) {
//    top[0]->Swap(*batch->data_);
//  } else {
//    top[0]->safe_reshape_mode(true);

//  tmp_.Reshape(batch->data_->shape());
//  tmp_.set_cpu_data(batch->data_->template mutable_cpu_data<Btype>());
//  boost::shared_ptr<Batch> top = processed_free_.pop();
//
//  top->data_->CopyDataFrom(tmp_, true, batch->data_packing(), transform_param_.forward_packing());
//  top->label_->Swap(*batch->label_);
//
//  processed_full_.push(top);

//  if (this->output_labels_) {
////    top[1]->Swap(*batch->label_);
//    top[1]->CopyDataFrom(*batch->label_);
//
////    LOG(INFO) <<top[1]->to_string();
//
//  }

//  batch->set_id((size_t) -1L);
//  prefetches_free_[next_batch_queue_]->push(batch);
//  next_batch_queue();







  try {
    while (!must_stop(0)) {
      shared_ptr<Batch> batch =
          prefetches_full_[next_batch_queue_]->pop("Data layer prefetch queue empty");
//      if (must_stop(0)) {
//        break;
//      }

      tmp_.Reshape(batch->data_->shape());
      tmp_.set_cpu_data(batch->data_->template mutable_cpu_data<Btype>());

      boost::shared_ptr<Batch> top = processed_free_.pop();
//      if (must_stop(0)) {
//        break;
//      }

      top->data_->CopyDataFrom(tmp_, true, batch->data_packing(), transform_param_.forward_packing());
      top->label_->Swap(*batch->label_);

      processed_full_.push(top);
      batch->set_id((size_t) -1L);
      prefetches_free_[next_batch_queue_]->push(batch);
      next_batch_queue();

    }
  }catch (boost::thread_interrupted&) {
  }

}

template<typename Ftype, typename Btype>
void BatchTransformer<Ftype, Btype>::reshape(const vector<int>& data_shape,
    const vector<int>& label_shape) {
  for (int i = 0; i < this->prefetch_.size(); ++i) {
    this->prefetch_[i]->data_->Reshape(data_shape);
    this->prefetch_[i]->label_->Reshape(label_shape);
  }
};


//
//template<typename Ftype, typename Btype>
//void BasePrefetchingDataLayer<Ftype, Btype>::ResizeQueues() {
//  size_t size = prefetches_free_.size();
//  if (queues_num_ > size) {
//    prefetches_free_.resize(queues_num_);
//    prefetches_full_.resize(queues_num_);
//    for (size_t i = size; i < queues_num_; ++i) {
//      shared_ptr<Batch> batch = make_shared<Batch>(tp<Ftype>(), tp<Ftype>());
//      prefetch_.push_back(batch);
//      prefetches_free_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
//      prefetches_full_[i] = make_shared<BlockingQueue<shared_ptr<Batch>>>();
//      prefetches_free_[i]->push(batch);
//    }
//  }

INSTANTIATE_CLASS_FB(BatchTransformer);

}
